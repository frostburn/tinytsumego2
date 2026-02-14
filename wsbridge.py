import ctypes
import sys

try:
  lib = ctypes.CDLL("./build/lib/libtinytsumego.so")
except OSError as e:
  sys.stderr.write("Library not found. Did you remember to compile it?\n")
  raise(e)

stones_t = ctypes.c_uint64

# enum move_result
ILLEGAL = 0
TARGET_LOST = 1
SECOND_PASS = 2
TAKE_TARGET = 3
CLEAR_KO = 4
TAKE_BUTTON = 5
PASS = 6
FILL_EXTERNAL = 7
NORMAL = 8
KO_THREAT_AND_RETAKE = 9

class State(ctypes.Structure):
  _fields_ = [
    ("visual_area", stones_t),
    ("logical_area", stones_t),
    ("player", stones_t),
    ("opponent", stones_t),
    ("ko", stones_t),
    ("target", stones_t),
    ("immortal", stones_t),
    ("external", stones_t),
    ("passes", ctypes.c_int),
    ("ko_threats", ctypes.c_int),
    ("button", ctypes.c_int),
    ("white_to_play", ctypes.c_bool),
    ("wide", ctypes.c_bool),
  ]

Version = ctypes.c_char * 16
version_str = Version()
lib.version(version_str)
version = version_str.value.decode()

print(f"Running Tinytsumego v{version}")

# Place-holder code to test that the lib works
s = State()
s.visual_area = lib.rectangle(4, 3)
s.logical_area = lib.rectangle(3, 2)
s.opponent = s.visual_area ^ s.logical_area
s.target = s.opponent

ps = ctypes.pointer(s)

r = lib.make_move(ps, lib.single(1, 1))
assert(r == NORMAL)
r = lib.make_move(ps, lib.single(1, 0))
assert(r == NORMAL)
r = lib.make_move(ps, lib.single(0, 1))
assert(r == NORMAL)
r = lib.make_move(ps, lib.single(2, 1))
assert(r == NORMAL)
r = lib.make_move(ps, lib.single(2, 0))
assert(r == TAKE_TARGET)
lib.print_state(ps)

# TODO: The actual WebSocket API
