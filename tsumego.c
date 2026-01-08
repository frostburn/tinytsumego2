// A collection of go problems to test the solvers
#include <string.h>
#include "tinytsumego2/state.h"

state get_tsumego(char *name) {
  state s;

  // . . . @
  // @ @ @ @
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

  // . . . . @
  // @ @ @ @ @
  s.visual_area = rectangle(5, 2);
  s.logical_area = rectangle(4, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Straight Four") == 0) {
    return s;
  }

  // . . . @
  // @ . @ @
  // @ @ @ @

  s.visual_area = rectangle(4, 3);
  s.logical_area = rectangle(3, 1) | single(1, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Hat Four") == 0) {
    return s;
  }

  s.player = s.opponent;
  s.opponent = 0;

  if (strcmp(name, "Hat Four Defense") == 0) {
    return s;
  }

  // . . . @ 0 0
  // . @ @ @ 0 0
  // @ @ @ @ 0 0

  s.visual_area = rectangle(6, 3);
  s.logical_area = rectangle(3, 1) | single(0, 1);
  s.player = rectangle(2, 3) << 4;
  s.opponent = rectangle(4, 3) ^ s.logical_area;
  s.immortal = s.player;
  s.target = s.opponent;

  if (strcmp(name, "Bent Four in the Corner") == 0) {
    return s;
  }

  s.ko_threats = -1;
  if (strcmp(name, "Bent Four in the Corner (1 ko threat)") == 0) {
    return s;
  }

  s.ko_threats = 0;
  s.logical_area ^= single(4, 0);
  s.player ^= single(4, 0);
  s.immortal = s.player;
  if (strcmp(name, "Bent Four in the Corner (1 liberty)") == 0) {
    return s;
  }

  s.logical_area ^= single(4, 1);
  s.player ^= single(4, 1);
  s.immortal = s.player;
  if (strcmp(name, "Bent Four in the Corner (2 liberties)") == 0) {
    return s;
  }

  // @ @ @ @ @
  // @ . . . @
  // @ . . @ @
  // @ @ @ @ @

  s.visual_area = rectangle(5, 4);
  s.logical_area = (rectangle(2, 2) << (1 + V_SHIFT)) | single(3, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;
  s.immortal = 0;
  s.ko_threats = 0;

  if (strcmp(name, "Bulky Five") == 0) {
    return s;
  }

  // . . . @ 0 0
  // . . . @ 0 0
  // @ @ @ @ 0 0

  s.visual_area = rectangle(6, 3);
  s.logical_area = rectangle(3, 2);
  s.player = rectangle(2, 3) << 4;
  s.opponent = rectangle(4, 3) ^ s.logical_area;
  s.target = s.opponent;
  s.immortal = s.player;
  s.ko_threats = -1;

  if (strcmp(name, "Rectangle Six") == 0) {
    return s;
  }

  s.logical_area ^= single(4, 0);
  s.immortal ^= single(4, 0);
  s.external ^= single(4, 0);
  s.ko_threats = 0;
  if (strcmp(name, "Rectangle Six (1 liberty)") == 0) {
    return s;
  }

  s.logical_area ^= single(4, 1);
  s.immortal ^= single(4, 1);
  s.external ^= single(4, 1);
  if (strcmp(name, "Rectangle Six (2 liberties)") == 0) {
    return s;
  }

  // Invalidate state if no matching entry found
  s.visual_area = 0;
  return s;
}
