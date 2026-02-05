import ctypes
import sys

try:
  lib = ctypes.CDLL("./build/lib/libtinytsumego.so")
except OSError as e:
  sys.stderr.write("Library not found. Did you remember to compile it?\n")
  raise(e)

Version = ctypes.c_char * 16
version_str = Version()
lib.version(version_str)
version = version_str.value.decode()

print(f"Running Tinytsumego v{version}")

# TODO: The actual WebSocket API
