// A collection of go problems to test the solvers
#include <string.h>
#include "tinytsumego2/state.h"

state get_tsumego(char *name) {
  state s;

  s.visual_area = rectangle(4, 2);
  s.logical_area = rectangle(3, 1);
  s.player = 0;
  s.opponent = rectangle(4, 2) ^ rectangle(3, 1);
  s.ko = 0;
  s.target = s.opponent;
  s.immortal = 0;
  s.passes = 0;
  s.ko_threats = 0;
  s.button = 0;
  s.white_to_play = false;

  if (strcmp(name, "Straight Three") == 0) {
    return s;
  }

  s.player = s.opponent;
  s.opponent = 0;

  if (strcmp(name, "Straight Three Defense") == 0) {
    return s;
  }

  // Invalidate state if no matching entry found
  s.visual_area = 0;
  return s;
}
