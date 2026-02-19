import ctypes

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

# stones.h bitboards
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

class Coordinates(ctypes.Structure):
  _fields_ = [
    ("x", ctypes.c_int),
    ("y", ctypes.c_int),
  ]

class Tsumego(ctypes.Structure):
  _fields_ = [
    ("slug", ctypes.c_char_p),
    ("subtitle", ctypes.c_char_p),
    ("state", State),
    ("bot_to_play", ctypes.c_bool),
    ("value", Value),
  ]

class Collection(ctypes.Structure):
  _fields_ = [
    ("slug", ctypes.c_char_p),
    ("title", ctypes.c_char_p),
    ("root", State),
    ("num_tsumegos", ctypes.c_size_t),
    ("tsumegos", ctypes.POINTER(Tsumego)),
  ]

  @property
  def tsumegos_by_slug(self):
    if not hasattr(self, "_tsumegos_by_slug"):
      self._tsumegos_by_slug = {}
      for i in range(self.num_tsumegos):
        t = self.tsumegos[i]
        self._tsumegos_by_slug[t.slug.decode()] = t
    return self._tsumegos_by_slug

class MoveInfo(ctypes.Structure):
  _fields_ = [
    ("coords", Coordinates),
    ("low_gain", ctypes.c_float),
    ("high_gain", ctypes.c_float),
    ("low_ideal", ctypes.c_bool),
    ("high_ideal", ctypes.c_bool),
    ("forcing", ctypes.c_bool),
  ]
