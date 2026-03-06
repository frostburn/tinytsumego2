from dotenv import load_dotenv
import ctypes
from ctypes import pointer
from lib_types import *
from lib_defs import lib
from handler import BaseHandler
import http.server
import json
import os
import sys
from urllib.parse import urlsplit
import threading

libc = ctypes.CDLL("libc.so.6")

MAX_BODY_SIZE = 64 * 1024

secret = 'Hunter2'

collection_slug = 'rectangle-six'
collection = None
reader = None
root = None

thread = None
httpd = None

def shutdown():
  httpd.shutdown()

class Handler(BaseHandler):
  """Handler for /api/tsumego/{collection_slug}"""

  def do_POST(self):
    if 'Content-Length' not in self.headers:
      self.json_response({"error": "Length required"}, 411)
      return
    try:
      length = int(self.headers.get('Content-Length', '0'))
    except ValueError:
      self.json_response({"error": "Invalid Content-Length"}, 400)
      return
    if length <= 0 or length > MAX_BODY_SIZE:
      self.json_response({"error": "Request body too large"}, 413)
      return
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
      low_terminal = lib.dual_graph_reader_low_terminal(reader, pointer(state), FORCING)
      dead_stones = lib.dead_stones(pointer(state), pointer(low_terminal))
      if dead_stones:
        high_terminal = lib.dual_graph_reader_high_terminal(reader, pointer(state), FORCING)
        high_stones = lib.dead_stones(pointer(state), pointer(high_terminal))
        dead_stones &= high_stones
      self.json_response({"deadStones": state.slice_stones(dead_stones)})
      return
    num_move_infos = ctypes.c_int(0)
    move_infos = lib.dual_graph_reader_move_infos(reader, pointer(state), pointer(num_move_infos))
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
    parsed = urlsplit(self.path)
    path = parsed.path.strip("/")
    if not path:
      data = {
        "title": collection.title.decode(),
        "root": collection.root.to_json(),
        "canStretch": collection.can_stretch,
        "tsumegos": [{"slug": t.slug.decode(), "subtitle": t.subtitle.decode()} for t in collection.tsumegos_by_slug.values()],
      }
      self.json_response(data)
      return
    else:
      tsumego_slug = path
      if tsumego_slug not in collection.tsumegos_by_slug:
        self.json_response({"error": f"Tsumego {tsumego_slug} not found"}, 404)
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

  def do_DELETE(self):
    if 'Authorization' in self.headers and self.headers['Authorization'] == secret:
      global thread
      print(f"Closing {collection_slug} server by request")
      self.send_response(204)
      self.end_headers()

      thread = threading.Thread(target=shutdown)
      thread.start()
    else:
      super().do_DELETE()

  def address_string(self):
    return collection_slug + ' ~ ' + super().address_string()

if __name__ == "__main__":
  if len(sys.argv) > 1:
    collection_slug = sys.argv[1]
  else:
    sys.stderr.write("collection slug not provided for prod_branch worker\n")
    sys.exit(1)
  if len(sys.argv) > 2:
    secret = sys.argv[2]
  load_dotenv()
  COLLECTION_PATH = os.getenv("COLLECTION_PATH")
  if COLLECTION_PATH is NONE:
    sys.stderr.write("COLLECTION_PATH not found in .env\n")
    sys.exit(1)

  filename = os.path.join(COLLECTION_PATH, f"{collection_slug}.bin")
  print("Reading", filename)
  reader = lib.allocate_dual_graph_reader(filename.encode())
  dummy = ctypes.c_int(0)
  root = State()
  lib.dual_graph_reader_python_stuff(reader, pointer(root), pointer(dummy))

  num_collections = ctypes.c_int(0)
  pc = lib.get_collections(pointer(num_collections))
  PORT = 8400
  for i in range(num_collections.value):
    slug = pc[i].slug.decode()
    if slug == collection_slug:
      print("Adding branch metadata for", slug)
      collection = pc[i]
      break
    PORT += 1

  if collection is None:
    sys.stderr.write(f"FATAL: metadata missing for {collection_slug}\n")
    sys.exit(1)

  print(f"Serving {collection_slug} at {PORT}")
  httpd = http.server.HTTPServer(("localhost", PORT), Handler)
  httpd.serve_forever()

  if thread is not None:
    thread.join()

  print(f"Closing {collection_slug} server...")
  httpd.server_close()

  print(f"Cleaning up {collection_slug}...")

  lib.unload_dual_graph_reader(reader)
  libc.free(reader)

  for i in range(num_collections.value):
    libc.free(pc[i].tsumegos)
  libc.free(pc)
