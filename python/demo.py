import ctypes
from lib_types import *
from lib_defs import lib

libc = ctypes.CDLL("libc.so.6")

# Random code to test that the lib loads correctly
if __name__ == '__main__':
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
