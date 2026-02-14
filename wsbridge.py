import ctypes
import sys

libc = ctypes.CDLL("libc.so.6")

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

# enum tactics
NONE = 0
DELAY = 1
FORCING = 2

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

class Value(ctypes.Structure):
  _fields_ = [
    ("low", ctypes.c_float),
    ("high", ctypes.c_float),
  ]

  def __repr__(self):
    return "Value({}, {})".format(self.low, self.high)

lib.rectangle.restype = stones_t
lib.allocate_dual_graph.restype = ctypes.c_void_p
lib.iterate_dual_graph.restype = ctypes.c_bool
lib.get_dual_graph_value.restype = Value
lib.moves_of.restype = ctypes.POINTER(stones_t)

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

lib.print_state(ps)

pdg = lib.allocate_dual_graph(ps)
while lib.iterate_dual_graph(pdg, True):
  pass
root_value = lib.get_dual_graph_value(pdg, ps, FORCING)
print(root_value)

num_moves = ctypes.c_int(0)
moves = lib.moves_of(ps, ctypes.pointer(num_moves))

value = lib.get_dual_graph_value(pdg, ps, NONE)
done = False
while not done:
  lib.print_state(ps)
  for i in range(num_moves.value):
    child = State.from_buffer_copy(s)
    pc = ctypes.pointer(child)
    r = lib.make_move(pc, moves[i])
    if r == TAKE_TARGET:
      s = child
      ps = pc
      done = True
      break
    child_value = lib.get_dual_graph_value(pdg, pc, NONE)
    if value.low == -child_value.high:
      s = child
      ps = pc
      value = child_value
      break

lib.print_state(ps)

lib.free_dual_graph(pdg)
libc.free(pdg)

# TODO: The actual WebSocket API
