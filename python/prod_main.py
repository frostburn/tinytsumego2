from dotenv import load_dotenv
import ctypes
from ctypes import pointer
from lib_types import *
from lib_defs import lib
import os
import sys
import subprocess
import http.client
import time
import secrets

libc = ctypes.CDLL("libc.so.6")

if __name__ == '__main__':
  load_dotenv()
  COLLECTION_PATH = os.getenv("COLLECTION_PATH")
  if COLLECTION_PATH is NONE:
    sys.stderr.write("COLLECTION_PATH not found in .env\n")
    sys.exit(1)

  print("Launching workers")
  secret = secrets.token_hex()
  print("Secret token =", secret)
  ps = []
  ps.append(subprocess.Popen([sys.executable, 'prod_root.py', secret]))

  binary_slugs = set()

  for filename in os.listdir(COLLECTION_PATH):
    [slug, ext] = os.path.splitext(filename)
    if ext != '.bin':
      continue
    binary_slugs.add(slug)
    ps.append(subprocess.Popen([sys.executable, 'prod_branch.py', slug, secret]))

  time.sleep(1)

  input('Press "Enter" to quit...\n')

  try:
    conn = http.client.HTTPConnection('localhost', 8361)
    conn.request("DELETE", "/", headers={"Authorization": secret})
    response = conn.getresponse()
    print(response.status, response.reason)
  except ConnectionError:
    print("Failed to communicate with root process")

  num_collections = ctypes.c_int(0)
  pc = lib.get_collections(pointer(num_collections))
  for i in range(num_collections.value):
    slug = pc[i].slug.decode()
    if slug in binary_slugs:
      try:
        conn = http.client.HTTPConnection('localhost', 8400 + i)
        conn.request("DELETE", "/", headers={"Authorization": secret})
        response = conn.getresponse()
        print(response.status, response.reason)
      except ConnectionError:
        print(f"Failed to communicate with {slug} process")
    else:
      print("Missing process for", slug)

  time.sleep(1)

  for p in ps:
    p.wait()

  print("Cleaning up main...")
  for i in range(num_collections.value):
    libc.free(pc[i].tsumegos)
  libc.free(pc)

  print("Done")
