// A collection of go problems to test the solvers
#include <string.h>
#include "tinytsumego2/state.h"

state get_tsumego(char *name) {
  stones_t temp;
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
  s.external = 0;
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

  // . . @
  // @ @ @
  s.visual_area = rectangle(3, 2);
  s.logical_area = rectangle(2, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Straight Two") == 0) {
    return s;
  }

  // . .
  s.visual_area = rectangle(2, 1);
  s.logical_area = rectangle(2, 1);
  s.player = 0;
  s.opponent = 0;
  s.target = 0;

  if (strcmp(name, "2x1 Goban") == 0) {
    return s;
  }

  s.button = 1;
  if (strcmp(name, "2x1 Goban (lost button)") == 0) {
    return s;
  }

  s.visual_area = rectangle(3, 1);
  s.logical_area = s.visual_area;
  s.button = 0;

  if (strcmp(name, "3x1 Goban") == 0) {
    return s;
  }

  s.visual_area = rectangle(4, 1);
  s.logical_area = s.visual_area;

  if (strcmp(name, "4x1 Goban") == 0) {
    return s;
  }

  s.visual_area = rectangle(5, 1);
  s.logical_area = s.visual_area;

  if (strcmp(name, "5x1 Goban") == 0) {
    return s;
  }

  s.visual_area = rectangle(2, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "2x2 Goban") == 0) {
    return s;
  }

  s.visual_area = rectangle(3, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "3x2 Goban") == 0) {
    return s;
  }

  s.visual_area = rectangle(4, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "4x2 Goban") == 0) {
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

  // . 0 . @
  // 0 @ @ @
  // . @ @ @
  // @ @ @ @

  s.visual_area = rectangle(4, 4);
  s.logical_area = rectangle(3, 1) | rectangle(1, 3);
  s.player = single(1, 0) | single(0, 1);
  s.opponent = s.visual_area ^ s.logical_area;
  s.immortal = 0;
  s.target = s.opponent;

  if (strcmp(name, "Bent Four in the Corner is Dead") == 0) {
    return s;
  }

  s.ko_threats = -1;
  if (strcmp(name, "Bent Four in the Corner is Dead (defender has threats)") == 0) {
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
  s.player ^= single(4, 0);
  s.ko_threats = 0;
  if (strcmp(name, "Rectangle Six (1 physical liberty)") == 0) {
    return s;
  }

  s.player ^= single(4, 0);
  s.external ^= single(4, 0);

  if (strcmp(name, "Rectangle Six (1 liberty)") == 0) {
    return s;
  }

  s.logical_area ^= single(4, 1);
  s.immortal ^= single(4, 1);
  s.external ^= single(4, 1);
  if (strcmp(name, "Rectangle Six (2 liberties)") == 0) {
    return s;
  }

  if (strcmp(name, "Problem A") == 0) {
    return parse_state("\
      , , B B B B B , , \
      B B B w w w w B , \
      . w w . . . w B , \
      . w . w . . w B , \
      B B w w . B B , , \
      , B B B B , , , , \
      , , , , , , , , , \
    ");
  }

  // . . . . @ 0
  // . . . . @ 0
  // @ @ @ @ @ 0

  s.visual_area = rectangle(6, 3);
  s.logical_area = rectangle(4, 2);
  s.player = rectangle(1, 3) << 5;
  s.opponent = rectangle(5, 3) ^ s.logical_area;
  s.target = s.opponent;
  s.immortal = s.player;
  s.external = 0;
  s.ko_threats = 0;

  if (strcmp(name, "Rectangle Eight") == 0) {
    return s;
  }

  s.ko_threats = -1;
  if (strcmp(name, "Rectangle Eight (defender has threats)") == 0) {
    return s;
  }

  // . . . @
  // . . . @
  // . . . @
  // @ @ @ @

  s.visual_area = rectangle(4, 4);
  s.logical_area = rectangle(3, 3);
  s.opponent = s.visual_area ^ s.logical_area;
  s.player = 0;
  s.target = s.opponent;
  s.immortal = 0;
  s.ko_threats = 0;

  if (strcmp(name, "Square Nine") == 0) {
    return s;
  }

  // . . . . . .
  // . . . @ 0 0
  // . . . @ 0 0
  // . @ @ @ 0 0
  // . 0 0 0 0 0
  // . 0 0 0 0 0

  s.visual_area = rectangle(6, 6);
  s.logical_area = rectangle(3, 3) | rectangle(6, 1) | rectangle(1, 6);
  s.opponent = rectangle(4, 4) & ~s.logical_area;
  s.player = rectangle(6, 6) & ~s.opponent & ~s.logical_area;
  s.target = s.opponent;
  s.immortal = s.player;
  s.ko_threats = 0;

  if (strcmp(name, "Carpenter's Square") == 0) {
    return s;
  }

  s.ko_threats = -2;
  if (strcmp(name, "Carpenter's Square (defender has threats)") == 0) {
    return s;
  }

  if (strcmp(name, "L group") == 0) {
    return parse_state("\
      . . . . . x x x x \
      W W b . . x x x x \
      , W b . . x x x x \
      , W b b . x x x x \
      W , W W . x x x x \
      , , , W . x x x x \
    ");
  }

  s = parse_state("   \
    . . . . . . x x x \
    W W b b . . x x x \
    , , W b . . x x x \
    , , W b b . x x x \
    , W , W W . x x x \
    , , , , W . x x x \
  ");

  if (strcmp(name, "First L+1 group defense") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "First L+1 group attack") == 0) {
    return s;
  }

  s = parse_state("     \
      . . . . . x x x x \
      W W b . . x x x x \
      , W b . . x x x x \
      , W b b . x x x x \
      W , W b . x x x x \
      , , , W . x x x x \
      , , , W . x x x x \
  ");
  s.ko_threats = -1;

  if (strcmp(name, "Second L+1 group defense") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = 0;
  if (strcmp(name, "Second L+1 group attack") == 0) {
    return s;
  }

  s = parse_state("     \
      W . . . . x x x x \
      W b b . . x x x x \
      , W b . . x x x x \
      , W b b . x x x x \
      W , W b . x x x x \
      , , , W . x x x x \
      , , , W . x x x x \
  ");

  if (strcmp(name, "L+2 group with descent defense") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = 1;
  if (strcmp(name, "L+2 group with descent attack") == 0) {
    return s;
  }

  s = parse_state("   \
    . . . . . x x x x \
    . . b W W x x x x \
    . . b W , x x x x \
    b . b W , x x x x \
    . b W , , x x x x \
    . W W , , x x x x \
    . W , W , x x x x \
  ");

  if (strcmp(name, "Basic J group defense") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "Basic J group attack") == 0) {
    return s;
  }

  s.logical_area ^= single(0, 3) ^ single(0, 4);
  s.opponent ^= single(0, 3) ^ single(0, 4);
  s.target = s.opponent;
  if (strcmp(name, "Straight J group attack") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "Straight J group defense") == 0) {
    return s;
  }

  s = parse_state("   \
    . . . . . . x x x \
    . . b b W W x x x \
    . . b W , , x x x \
    b . b W , , x x x \
    . b W , , , x x x \
    W W W , , , x x x \
  ");

  if (strcmp(name, "J+1 group with descent defense") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = -1;
  if (strcmp(name, "J+1 group with descent attack") == 0) {
    return s;
  }

  s = parse_state("\
      . . . . . x x x x \
      W W b . . x x x x \
      , W b . . x x x x \
      , W b . . x x x x \
      , W b b . x x x x \
      W , W W . x x x x \
      , , , W . x x x x \
  ");

  if (strcmp(name, "Long L group defense") == 0) {
    return s;
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;

  if (strcmp(name, "Long L group attack") == 0) {
    return s;
  }

  s.ko_threats = 1;
  if (strcmp(name, "Long L group attack (with threats)") == 0) {
    return s;
  }

  // Invalidate state if no matching entry found
  s.visual_area = 0;
  return s;
}
