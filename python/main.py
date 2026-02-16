import ctypes
from lib_types import *
from lib_defs import lib
import http.server
import json
import os
import sys

libc = ctypes.CDLL("libc.so.6")

# Global config is bad, but let's just get this thing off the ground
allow_origin = None
collection_path = None
readers = {}
collections = {}

class Handler(http.server.BaseHTTPRequestHandler):
  def write_json(self, data):
    self.wfile.write((json.dumps(data) + '\n').encode("utf-8"))

  def json_response(self, data):
    self.send_response(200)
    if allow_origin:
      self.send_header("Access-Control-Allow-Origin", allow_origin)
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
    path = self.path.strip("/")
    parts = path.split("/")
    if len(parts) != 2 or parts[0] != "tsumego":
      self.send_response(405)
      if allow_origin:
        self.send_header("Access-Control-Allow-Origin", allow_origin)
      self.send_header("Content-type", "application/json")
      self.end_headers()
      self.write_json({"error": f"POST not allowed"})
      return
    collection_slug = parts[1]
    if collection_slug not in collections:
      self.send_response(404)
      if allow_origin:
        self.send_header("Access-Control-Allow-Origin", allow_origin)
      self.send_header("Content-type", "application/json")
      self.end_headers()
      self.write_json({"error": f"Collection {collection_slug} not found"})
      return
    collection = collections[collection_slug]
    (reader, root) = readers[collection_slug]
    content_length = int(self.headers['Content-Length'])
    content = self.rfile.read(content_length).decode("utf-8")
    data = json.loads(content)
    state = State.from_json(data["state"], root.wide)
    num_move_infos = ctypes.c_int(0)
    move_infos = lib.dual_graph_reader_move_infos(reader, ctypes.pointer(state), ctypes.pointer(num_move_infos))
    response_data = {"moves": []}
    for i in range(num_move_infos.value):
      response_data["moves"].append({
        "x": move_infos[i].coords.x,
        "y": move_infos[i].coords.y,
        "lowGain": move_infos[i].low_gain,
        "highGain": move_infos[i].high_gain,
        "lowIdeal": move_infos[i].low_ideal,
        "highIdeal": move_infos[i].high_ideal,
        "forcing": move_infos[i].forcing,
      })
    libc.free(move_infos)
    self.json_response(response_data)

  def do_GET(self):
    path = self.path.strip("/")
    if path == "":
      self.send_response(200)
      if allow_origin:
        self.send_header("Access-Control-Allow-Origin", allow_origin)
      self.send_header("Content-type", "text/html")
      self.end_headers()
      self.wfile.write(b"""
        <html><head><title>TinyTsumego JSON API</title></head><body>
          <p>This is a the root of the TinyTsumego JSON API</p>
        </body></html>
      \n""")
      return
    parts = path.split("/")
    if parts[0] == "tsumego":
      if len(parts) == 1:
        data = {
          "collections": [{"slug": c.slug.decode(), "title": c.title.decode()} for c in collections.values()],
        }
        self.json_response(data)
        return
      elif len(parts) >= 2:
        collection_slug = parts[1]
        if collection_slug not in collections:
          self.send_response(404)
          if allow_origin:
            self.send_header("Access-Control-Allow-Origin", allow_origin)
          self.send_header("Content-type", "application/json")
          self.end_headers()
          self.write_json({"error": f"Collection {collection_slug} not found"})
          return
      collection = collections[collection_slug]
      if len(parts) == 2:
        data = {
          "title": collection.title.decode(),
          "root": collection.root.to_json(),
          "tsumegos": [{"slug": t.slug.decode(), "subtitle": t.subtitle.decode()} for t in collection.tsumegos_by_slug.values()],
        }
        self.json_response(data)
        return
      elif len(parts) == 3:
        tsumego_slug = parts[2]
        if tsumego_slug not in collection.tsumegos_by_slug:
          self.send_response(404)
          if allow_origin:
            self.send_header("Access-Control-Allow-Origin", allow_origin)
          self.send_header("Content-type", "application/json")
          self.end_headers()
          self.write_json({"error": f"Tsumego {tsumego_slug} not found"})
          return
        tsumego = collection.tsumegos_by_slug[tsumego_slug]
        data = {
          "title": collection.title.decode(),
          "subtitle": tsumego.subtitle.decode(),
          "state": tsumego.state.to_json(),
          "botToPlay": tsumego.bot_to_play,
        }
        self.json_response(data)
        return
    self.send_response(404)
    if allow_origin:
      self.send_header("Access-Control-Allow-Origin", allow_origin)
    self.send_header("Content-type", "application/json")
    self.end_headers()
    self.write_json({"error": f"Resource not found"})
    return

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

  if len(sys.argv) > 2 and sys.argv[2] == '--dev':
    print("Dev mode enabled: Access-Control-Allow-Origin = '*'")
    allow_origin = "*"

  for filename in os.listdir(collection_path):
    print("Reading", filename)
    reader = lib.allocate_dual_graph_reader(os.path.join(collection_path, filename).encode())
    dummy = ctypes.c_int(0)
    root = State()
    lib.dual_graph_reader_python_stuff(reader, ctypes.pointer(root), ctypes.pointer(dummy))
    readers[filename.strip(".bin")] = (reader, root)

  num_collections = ctypes.c_int(0)
  pc = lib.get_collections(ctypes.pointer(num_collections))
  for i in range(num_collections.value):
    slug = pc[i].slug.decode()
    if slug in readers:
      print("Adding metadata for", slug)
      collections[slug] = pc[i]

  server = http.server.HTTPServer(("localhost", 8361), Handler)
  try:
    server.serve_forever()
  except KeyboardInterrupt:
    print()
    pass

  print("Closing server...")
  server.server_close()

  print("Cleaning up...")
  for (reader, _) in readers.values():
    lib.unload_dual_graph_reader(reader)
    libc.free(reader)

  for i in range(num_collections.value):
    libc.free(pc[i].tsumegos)
  libc.free(pc)
