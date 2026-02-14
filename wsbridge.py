import ctypes
import json
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

WIDTH = 9
NORTH_WALL = (1 << WIDTH) - 1
V_SHIFT = WIDTH

WIDTH_16 = 16
NORTH_WALL_16 = (1 << WIDTH_16) - 1
V_SHIFT_16 = WIDTH_16

def unslice_stones(slices):
  result = 0
  while slices:
    result = (result << V_SHIFT) | (slices.pop() & NORTH_WALL)
  return result

def unslice_stones_16(slices):
  result = 0
  while slices:
    result = (result << V_SHIFT_16) | (slices.pop() & NORTH_WALL_16)
  return result

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

  @classmethod
  def from_json(cls, data, wide):
    unslice = unslice_stones_16 if wide else unslice_stones
    return cls(
      unslice(data["visualArea"]),
      unslice(data["logicalArea"]),
      unslice(data["player"]),
      unslice(data["opponent"]),
      unslice(data["ko"]),
      unslice(data["target"]),
      unslice(data["immortal"]),
      unslice(data["external"]),
      data["passes"],
      data["koThreats"],
      data["button"],
      data["whiteToPlay"],
      wide
    )

  def slice_stones(self, stones):
    wall = NORTH_WALL_16 if self.wide else NORTH_WALL
    shift = V_SHIFT_16 if self.wide else V_SHIFT
    result = []
    while stones:
      result.append(stones & wall)
      stones >>= shift
    return result

  def to_json(self):
    return {
      "visualArea": self.slice_stones(self.visual_area),
      "logicalArea": self.slice_stones(self.logical_area),
      "player": self.slice_stones(self.player),
      "opponent": self.slice_stones(self.opponent),
      "ko": self.slice_stones(self.ko),
      "target": self.slice_stones(self.target),
      "immortal": self.slice_stones(self.immortal),
      "external": self.slice_stones(self.external),
      "passes": self.passes,
      "koThreats": self.ko_threats,
      "button": self.button,
      "whiteToPlay": self.white_to_play,
    }

class Value(ctypes.Structure):
  _fields_ = [
    ("low", ctypes.c_float),
    ("high", ctypes.c_float),
  ]

  def __repr__(self):
    return "Value({}, {})".format(self.low, self.high)

class DualValue(ctypes.Structure):
  _fields_ = [
    ("plain", Value),
    ("forcing", Value),
  ]

  def __repr__(self):
    return "DualValue({}, {})".format(self.plain, self.forcing)

class Coordinates(ctypes.Structure):
  _fields_ = [
    ("x", ctypes.c_int),
    ("y", ctypes.c_int),
  ]

# stones.h
lib.rectangle.restype = stones_t
lib.coords_of.restype = Coordinates
lib.coords_of_16.restype = Coordinates
# state.h
lib.moves_of.restype = ctypes.POINTER(stones_t)
# dual_solver.h
lib.allocate_dual_graph.restype = ctypes.c_void_p
lib.iterate_dual_graph.restype = ctypes.c_bool
lib.get_dual_graph_value.restype = Value
# dual_reader.h
lib.allocate_dual_graph_reader.restype = ctypes.c_void_p
lib.get_dual_graph_reader_value.restype = DualValue
lib.dual_graph_reader_python_stuff.restype = ctypes.POINTER(stones_t)
# scoring.h
lib.score_terminal.restype = Value
lib.apply_tactics.restype = Value

def collapse_low_principal(principal):
  if principal:
    result = [{"x": principal["x"], "y": principal["y"]}]
    result.extend(collapse_high_principal(principal["highPrincipal"]))
    return result
  return []

def collapse_high_principal(principal):
  if principal:
    result = [{"x": principal["x"], "y": principal["y"]}]
    result.extend(collapse_low_principal(principal["lowPrincipal"]))
    return result
  return []

class GraphNavigator:
  def __init__(self, reader):
    self.reader = reader
    self.root = State()
    self.num_moves = ctypes.c_int(0)
    self.moves = lib.dual_graph_reader_python_stuff(reader, ctypes.pointer(self.root), ctypes.pointer(self.num_moves))
    self.coordss = []
    coords_of = lib.coords_of_16 if self.root.wide else lib.coords_of
    for i in range(self.num_moves.value):
      self.coordss.append(coords_of(self.moves[i]))

  def _get_info(self, state, get_low, get_high):
    ps = ctypes.pointer(state)
    value = lib.get_dual_graph_reader_value(self.reader, ps)
    moves = []
    child_values = []
    for i in range(len(self.coordss)):
      child = State.from_buffer_copy(state)
      pc = ctypes.pointer(child)
      r = lib.make_move(pc, self.moves[i])
      if r == ILLEGAL:
        continue

      info = {
        "x": self.coordss[i].x,
        "y": self.coordss[i].y,
      }
      if r <= TAKE_TARGET:
        child_value = DualValue()
        child_value.plain = lib.score_terminal(r, pc)
        child_value.forcing = child_value.plain
        child_values.append(child_value)
        info["lowPrincipal"] = None
        info["highPrincipal"] = None
      else:
        child_value = lib.get_dual_graph_reader_value(self.reader, pc)
        child_value.plain = lib.apply_tactics(NONE, r, pc, child_value.plain)
        child_values.append(child_value)
        if get_low:
          info |= self._get_info(child, False, True)
        if get_high:
          childPrincipal = info.get("highPrincipal")  # Work around `get_low = True`
          info |= self._get_info(child, True, False)
          info["highPrincipal"] = childPrincipal
      moves.append(info)

    lows_high = float("-inf")
    highs_low = float("-inf")
    for child_value in child_values:
      if value.plain.low == child_value.plain.high:
        lows_high = max(lows_high, child_value.plain.low)
      if value.plain.high == child_value.plain.low:
        highs_low = max(highs_low, child_value.plain.high)

    lowPrincipal = None
    highPrincipal = None
    for i in range(len(moves)):
      move = moves[i]
      child_value = child_values[i]
      if value.plain.low == child_value.plain.high and lows_high == child_value.plain.low:
        move["lowIdeal"] = True
        if get_low and lowPrincipal is None:
          lowPrincipal = move
      else:
        move["lowIdeal"] = False
      if value.plain.high == child_value.plain.low and highs_low == child_value.plain.high:
        move["highIdeal"] = True
        if get_high and highPrincipal is None:
          highPrincipal = move
      else:
        move["highIdeal"] = False

    return {
      "lowPrincipal": lowPrincipal,
      "highPrincipal": highPrincipal,
      "low": value.plain.low,
      "high": value.plain.high,
    }

  def get_info(self, state):
    ps = ctypes.pointer(state)
    value = lib.get_dual_graph_reader_value(self.reader, ps)
    moves = []
    forcing_moves = []
    child_values = []
    for i in range(len(self.coordss)):
      child = State.from_buffer_copy(state)
      pc = ctypes.pointer(child)
      r = lib.make_move(pc, self.moves[i])
      if r == ILLEGAL:
        continue

      info = {
        "x": self.coordss[i].x,
        "y": self.coordss[i].y,
      }
      if r <= TAKE_TARGET:
        child_value = DualValue()
        child_value.plain = lib.score_terminal(r, pc)
        child_value.forcing = child_value.plain
        child_values.append(child_value)
        info["lowPrincipal"] = None
        info["highPrincipal"] = None
      else:
        child_value = lib.get_dual_graph_reader_value(self.reader, pc)
        child_value.plain = lib.apply_tactics(NONE, r, pc, child_value.plain)
        child_value.forcing = lib.apply_tactics(FORCING, r, pc, child_value.forcing)
        child_values.append(child_value)
        info |= self._get_info(child, True, True)
      moves.append(info)

    lows_high = float("-inf")
    highs_low = float("-inf")
    for child_value in child_values:
      if value.plain.low == child_value.plain.high:
        lows_high = max(lows_high, child_value.plain.low)
      if value.plain.high == child_value.plain.low:
        highs_low = max(highs_low, child_value.plain.high)

    lowPrincipal = None
    highPrincipal = None
    forcing_moves = []
    for i in range(len(moves)):
      move = moves[i]
      child_value = child_values[i]
      move["lowGain"] = child_value.plain.high - value.plain.low
      move["highGain"] = child_value.plain.low - value.plain.high
      if value.plain.low == child_value.plain.high and lows_high == child_value.plain.low:
        move["lowIdeal"] = True
        if lowPrincipal is None:
          lowPrincipal = move
      else:
        move["lowIdeal"] = False
      if value.plain.high == child_value.plain.low and highs_low == child_value.plain.high:
        move["highIdeal"] = True
        if highPrincipal is None:
          highPrincipal = move
      else:
        move["highIdeal"] = False

      if value.forcing.low == child_value.forcing.high:
        forcing_moves.append({"x": move["x"], "y": move["y"]})

    lowPrincipal = collapse_low_principal(lowPrincipal)
    highPrincipal = collapse_high_principal(highPrincipal)

    for move in moves:
      move["lowPrincipal"] = collapse_low_principal(move["lowPrincipal"])
      move["highPrincipal"] = collapse_high_principal(move["highPrincipal"])

    return {
      "moves": moves,
      "lowPrincipal": lowPrincipal,
      "highPrincipal": highPrincipal,
      "low": value.plain.low,
      "high": value.plain.high,
      "forcingMoves": forcing_moves,
    }

Version = ctypes.c_char * 16
version_str = Version()
lib.version(version_str)
version = version_str.value.decode()

print(f"Running Tinytsumego v{version}")

# Place-holder code to test that the lib works


if len(sys.argv) > 1:
  filename = ctypes.c_char_p(sys.argv[1].encode())
  pdgr = lib.allocate_dual_graph_reader(filename)

  navigator = GraphNavigator(pdgr)
  import pprint
  data = navigator.root.to_json()
  pprint.pp(data)
  root = State.from_json(data, navigator.root.wide)
  lib.print_state(ctypes.pointer(root))
  pprint.pp(navigator.get_info(navigator.root), indent=1)

  lib.unload_dual_graph_reader(pdgr)
  libc.free(pdgr)

else:
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

# TODO: Rename file and convert to a HTTP server
