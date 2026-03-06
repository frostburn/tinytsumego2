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

secret = 'Hunter2'
collections = {}

thread = None
httpd = None

def shutdown():
  httpd.shutdown()

class Handler(BaseHandler):
  """Handler for /api/ and /api/tsumego/"""

  def do_POST(self):
    self.json_response({"error": "POST not allowed"}, 405)

  def do_GET(self):
    parsed = urlsplit(self.path)
    path = parsed.path.strip("/")
    query = parsed.query
    if path == "":
      self.send_response(200)
      self.send_default_headers()
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
        if query == "deep=1":
          data = {
            "collections": [{"slug": c.slug.decode(), "tsumegos": list(c.tsumegos_by_slug.keys())} for c in collections.values()]
          }
        else:
          data = {
            "collections": [{"slug": c.slug.decode(), "title": c.title.decode()} for c in collections.values()],
          }
        self.json_response(data)
        return
      elif len(parts) >= 2:
        collection_slug = parts[1]
        if collection_slug not in collections:
          self.json_response({"error": f"Collection {collection_slug} not found"}, 404)
          return
      self.json_response({"error": "Unexpected GET hit at prod_root"}, 500)
      return
    self.json_response({"error": f"Resource not found"}, 404)
    return

  def do_DELETE(self):
    if 'Authorization' in self.headers and self.headers['Authorization'] == secret:
      global thread
      print("Shutting down root server by request")
      self.send_response(204)
      self.end_headers()

      thread = threading.Thread(target=shutdown)
      thread.start()
    else:
      super().do_DELETE()


if __name__ == "__main__":
  if len(sys.argv) > 1:
    secret = sys.argv[1]
  load_dotenv()
  COLLECTION_PATH = os.getenv("COLLECTION_PATH")
  if COLLECTION_PATH is NONE:
    sys.stderr.write("COLLECTION_PATH not found in .env\n")
    sys.exit(1)

  binary_slugs = set()

  for filename in os.listdir(COLLECTION_PATH):
    [slug, ext] = os.path.splitext(filename)
    if ext != '.bin':
      continue
    binary_slugs.add(slug)

  num_collections = ctypes.c_int(0)
  pc = lib.get_collections(pointer(num_collections))
  for i in range(num_collections.value):
    slug = pc[i].slug.decode()
    if slug in binary_slugs:
      print("Adding metadata for", slug)
      collections[slug] = pc[i]
    else:
      print("Missing binary for", slug)

  PORT = 8361
  print(f"Serving root at {PORT}")
  httpd = http.server.HTTPServer(("localhost", PORT), Handler)
  httpd.serve_forever()

  if thread is not None:
    thread.join()

  print("Closing root server...")
  httpd.server_close()

  print("Cleaning up root...")
  for i in range(num_collections.value):
    libc.free(pc[i].tsumegos)
  libc.free(pc)
