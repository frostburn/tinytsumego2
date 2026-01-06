#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

int main() {
  printf("Hello, World!\n");
  print_stones(NORTH_WALL);

  state s;
  s.visual_area = rectangle(3, 2);
  s.logical_area = s.visual_area;
  s.player = 0;
  s.opponent = 0;
  s.ko = 0;
  s.target = 0;
  s.immortal = 0;
  s.passes = 0;
  s.ko_threats = 0;
  s.button = 0;

  print_state(&s, false);

  make_move(&s, single(0, 0));
  print_state(&s, true);

  make_move(&s, single(1, 0));
  print_state(&s, false);

  make_move(&s, pass());
  print_state(&s, true);

  make_move(&s, single(0, 1));
  print_state(&s, false);

  printf("thx bye\n");
  return EXIT_SUCCESS;
}
