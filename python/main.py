import ctypes
from ctypes import pointer
from lib_types import *
from lib_defs import lib
import http.server
import json
import os
import sys
from urllib.parse import urlsplit

libc = ctypes.CDLL("libc.so.6")

MAX_BODY_SIZE = 64 * 1024

# Global config is bad, but let's just get this thing off the ground
dev_mode = False
allow_origin = None
collection_path = None
readers = {}
collections = {}


class Handler(http.server.BaseHTTPRequestHandler):
    def send_default_headers(self):
        if allow_origin:
            self.send_header("Access-Control-Allow-Origin", allow_origin)
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")

    def write_json(self, data):
        self.wfile.write((json.dumps(data) + "\n").encode("utf-8"))

    def json_response(self, data, code=200):
        self.send_response(code)
        self.send_default_headers()
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.write_json(data)

    def do_OPTIONS(self):
        self.send_response(204)
        if allow_origin:
            self.send_header("Access-Control-Allow-Origin", allow_origin)
        self.send_header("Allow", "OPTIONS, GET, POST")
        self.send_header("Access-Control-Allow-Headers", "Content-type")
        self.end_headers()

    def do_POST(self):
        if "Content-Length" not in self.headers:
            self.json_response({"error": "Length required"}, 411)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.json_response({"error": "Invalid Content-Length"}, 400)
            return
        if length <= 0 or length > MAX_BODY_SIZE:
            self.json_response({"error": "Request body too large"}, 413)
            return
        parsed = urlsplit(self.path)
        path = parsed.path.strip("/")
        parts = path.split("/")
        if len(parts) != 2 or parts[0] != "tsumego":
            self.json_response({"error": "POST not allowed"}, 405)
            return
        collection_slug = parts[1]
        if collection_slug not in collections:
            self.json_response(
                {"error": f"Collection {collection_slug} not found"}, 404
            )
            return
        collection = collections[collection_slug]
        (reader, root) = readers[collection_slug]
        content = self.rfile.read(length).decode("utf-8")
        try:
            data = json.loads(content)
        except json.JSONDecodeError:
            self.json_response({"error": "Malformed JSON body"}, 400)
            return
        if "state" not in data:
            self.json_response({"error": 'Missing "state" field'}, 400)
            return
        state = State.from_json(data["state"], root.wide)
        if state.passes >= 2:
            state.passes = 0
            state.ko = 0
            state.ko_threats = 0
            # Plain values have been "used up" for area scoring. Take the forcing terminal.
            low_terminal = lib.dual_graph_reader_low_terminal(
                reader, pointer(state), FORCING
            )
            dead_stones = lib.dead_stones(pointer(state), pointer(low_terminal))
            if dead_stones:
                high_terminal = lib.dual_graph_reader_high_terminal(
                    reader, pointer(state), FORCING
                )
                high_stones = lib.dead_stones(pointer(state), pointer(high_terminal))
                dead_stones &= high_stones
            self.json_response({"deadStones": state.slice_stones(dead_stones)})
            return
        num_move_infos = ctypes.c_int(0)
        move_infos = lib.dual_graph_reader_move_infos(
            reader, pointer(state), pointer(num_move_infos)
        )
        response_data = {"moves": []}
        for i in range(num_move_infos.value):
            response_data["moves"].append(
                {
                    "x": move_infos[i].coords.x,
                    "y": move_infos[i].coords.y,
                    "lowGain": move_infos[i].low_gain,
                    "highGain": move_infos[i].high_gain,
                    "lowIdeal": move_infos[i].low_ideal,
                    "highIdeal": move_infos[i].high_ideal,
                    "forcing": move_infos[i].forcing,
                }
            )
        libc.free(move_infos)
        if dev_mode:
            lib.print_state(pointer(state))
            normalized = lib.strip_aesthetics(reader, pointer(state))
            v = lib.get_dual_graph_reader_value(reader, pointer(normalized))
            print(f"Plain: {v.plain.low}, {v.plain.high}")
            print(f"Forcing: {v.forcing.low}, {v.forcing.high}")
        self.json_response(response_data)

    def do_GET(self):
        parsed = urlsplit(self.path)
        path = parsed.path.strip("/")
        query = parsed.query
        if path == "":
            self.send_response(200)
            self.send_default_headers()
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(
                b"""
                    <html><head><title>TinyTsumego JSON API</title></head><body>
                    <p>This is a the root of the TinyTsumego JSON API</p>
                    </body></html>\n
                """
            )
            return
        parts = path.split("/")
        if parts[0] == "tsumego":
            if len(parts) == 1:
                if query == "deep=1":
                    data = {
                        "collections": [
                            {
                                "slug": c.slug.decode(),
                                "tsumegos": list(c.tsumegos_by_slug.keys()),
                            }
                            for c in collections.values()
                        ]
                    }
                else:
                    data = {
                        "collections": [
                            {"slug": c.slug.decode(), "title": c.title.decode()}
                            for c in collections.values()
                        ],
                    }
                self.json_response(data)
                return
            elif len(parts) >= 2:
                collection_slug = parts[1]
                if collection_slug not in collections:
                    self.json_response(
                        {"error": f"Collection {collection_slug} not found"}, 404
                    )
                    return
            collection = collections[collection_slug]
            if len(parts) == 2:
                data = {
                    "title": collection.title.decode(),
                    "root": collection.root.to_json(),
                    "canStretch": collection.can_stretch,
                    "tsumegos": [
                        {"slug": t.slug.decode(), "subtitle": t.subtitle.decode()}
                        for t in collection.tsumegos_by_slug.values()
                    ],
                }
                self.json_response(data)
                return
            elif len(parts) == 3:
                tsumego_slug = parts[2]
                if tsumego_slug not in collection.tsumegos_by_slug:
                    self.json_response(
                        {"error": f"Tsumego {tsumego_slug} not found"}, 404
                    )
                    return
                tsumego = collection.tsumegos_by_slug[tsumego_slug]
                data = {
                    "title": collection.title.decode(),
                    "subtitle": tsumego.subtitle.decode(),
                    "state": tsumego.state.to_json(),
                    "botToPlay": tsumego.bot_to_play,
                    "canStretch": collection.can_stretch,
                }
                self.json_response(data)
                return
        self.json_response({"error": f"Resource not found"}, 404)
        return

    def do_PUT(self):
        parsed = urlsplit(self.path)
        path = parsed.path.strip("/")
        if path == "coffee":
            self.json_response(
                {
                    "error": "Do not PUT coffee grounds into a teapot, which I am, by the way..."
                },
                418,
            )
            return
        self.json_response({"error": "PUT not allowed"}, 405)

    def do_DELETE(self):
        self.json_response({"error": "DELETE not allowed"}, 405)


Version = ctypes.c_char * 16
version_str = Version()
lib.version(version_str)
version = version_str.value.decode()

if __name__ == "__main__":
    print(f"Running TinyTsumego v{version}")

    if len(sys.argv) > 1:
        collection_path = sys.argv[1]
    else:
        sys.stderr.write("A path to generated collections must be provided\n")
        sys.exit(1)

    if len(sys.argv) > 2 and sys.argv[2] == "--dev":
        dev_mode = True
        print("Dev mode enabled: Access-Control-Allow-Origin = '*'")
        allow_origin = "*"

    for filename in os.listdir(collection_path):
        [slug, ext] = os.path.splitext(filename)
        if ext != ".bin":
            continue
        print("Reading", filename)
        reader = lib.allocate_dual_graph_reader(
            os.path.join(collection_path, filename).encode()
        )
        dummy = ctypes.c_int(0)
        root = State()
        lib.dual_graph_reader_python_stuff(reader, pointer(root), pointer(dummy))
        readers[slug] = (reader, root)

    num_collections = ctypes.c_int(0)
    pc = lib.get_collections(pointer(num_collections))
    for i in range(num_collections.value):
        slug = pc[i].slug.decode()
        if slug in readers:
            print("Adding metadata for", slug)
            collections[slug] = pc[i]
        else:
            print("Missing binary for", slug)

    server = http.server.HTTPServer(("localhost", 8361), Handler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print()
        pass

    print("Closing server...")
    server.server_close()

    print("Cleaning up...")
    for reader, _ in readers.values():
        lib.unload_dual_graph_reader(reader)
        libc.free(reader)

    for i in range(num_collections.value):
        libc.free(pc[i].tsumegos)
    libc.free(pc)
