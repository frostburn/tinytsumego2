import ctypes
from lib_types import *

try:
  lib = ctypes.CDLL("../build/lib/libtinytsumego.so")
except OSError as e:
  sys.stderr.write("Library not found. Did you remember to compile it?\n")
  raise(e)

# stones.h
lib.rectangle.restype = stones_t
lib.coords_of.restype = Coordinates
lib.coords_of_16.restype = Coordinates
# state.h
lib.moves_of.restype = ctypes.POINTER(stones_t)
lib.equals.restype = ctypes.c_bool
lib.compensated_liberty_score.restype = ctypes.c_int
# compressed_graph.h
lib.allocate_compressed_graph.restype = ctypes.c_void_p
lib.iterate_compressed_graph.restype = ctypes.c_bool
lib.get_compressed_graph_value.restype = Value
# compressed_reader.h
lib.allocate_compressed_graph_reader.restype = ctypes.c_void_p
lib.get_compressed_graph_reader_value.restype = Value
lib.compressed_graph_reader_python_stuff.restype = ctypes.POINTER(stones_t)
lib.compressed_graph_reader_move_infos.restype = ctypes.POINTER(MoveInfo)
# scoring.h
lib.score_terminal.restype = Value
lib.apply_tactics.restype = Value
# collection.h
lib.get_collections.restype = ctypes.POINTER(Collection)
