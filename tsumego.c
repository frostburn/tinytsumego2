// A collection of go problems to test the solvers
#include <string.h>
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/state.h"

typedef struct tsumego {
  state state;
  float low;
  float high;
  float low_delay;
  float high_delay;
} tsumego;

static const char* TSUMEGO_NAMES[] = {
  "Straight Three",
  "Straight Three Defense",
  "Straight Two",
  "2x1 Goban",
  "2x1 Goban (lost button)",
  "3x1 Goban",
  "4x1 Goban",
  "5x1 Goban",
  "5x1 Goban",
  "2x2 Goban",
  "3x2 Goban",
  "4x2 Goban",
  "Straight Four",
  "Hat Four",
  "Hat Four Defense",
  "Bent Four in the Corner",
  "Bent Four in the Corner (1 ko threat)",
  "Bent Four in the Corner (1 liberty)",
  "Bent Four in the Corner (2 liberties)",
  "Bent Four in the Corner is Dead",
  "Bent Four in the Corner is Dead (defender has threats)",
  "Bent Four in the Corner is Dead (attacker tenuki)",
  "Bulky Five",
  "Rectangle Six in the Corner",
  "Rectangle Six in the Corner (1 physical liberty)",
  "Rectangle Six in the Corner (1 liberty)",
  "Rectangle Six in the Corner (2 liberties)",
  "Problem A",
  "Rectangle Eight in the Corner",
  "Rectangle Eight in the Corner (defender has threats)",
  "Square Nine in the Corner",
  // "Carpenter's Square",  // TODO: Tweak layout
  // "Carpenter's Square (defender has threats)",  // TODO: Tweak layout
  "L Group",
  "First L+1 Group Defense",
  "First L+1 Group Attack",
  "Second L+1 Group Defense",
  "Second L+1 Group Attack",
  "L+2 Group with Descent Defense",
  "L+2 Group with Descent Attack",
  "Basic J Group Defense",
  "Basic J Group Attack",
  "Straight J Group Attack",
  "Straight J Group Defense",
  "J+1 Group with Descent Defense",
  "J+1 Group with Descent Attack",
  "Long L Group Defense",
  "Long L Group Attack",
  "Long L Group Attack (with threats)",
};

const size_t NUM_TSUMEGO = sizeof(TSUMEGO_NAMES) / sizeof(char*);

tsumego single_valued(state s, float score) {
  return (tsumego) {s, score, score, score, score};
}

tsumego delay_valued(state s, float score, float delay_score) {
  return (tsumego) {s, score, score, delay_score, delay_score};
}

tsumego dual_valued(state s, float low, float high) {
  return (tsumego) {s, low, high, low, high};
}

tsumego get_tsumego(const char *name) {
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 998.75);
  }

  s.player = s.opponent;
  s.opponent = 0;

  if (strcmp(name, "Straight Three Defense") == 0) {
    return single_valued(s, 7.5);
  }

  // . . @
  // @ @ @
  s.visual_area = rectangle(3, 2);
  s.logical_area = rectangle(2, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Straight Two") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE + 0.5, 1000);
  }

  // . .
  s.visual_area = rectangle(2, 1);
  s.logical_area = rectangle(2, 1);
  s.player = 0;
  s.opponent = 0;
  s.target = 0;

  if (strcmp(name, "2x1 Goban") == 0) {
    // Score: -1.5 to 2.5
    return dual_valued(s, -1.5, 2.5);
  }

  s.button = 1;
  if (strcmp(name, "2x1 Goban (lost button)") == 0) {
    // Score: -1.5 to 2.5
    return dual_valued(s, -1.5, 2.5);
  }

  s.visual_area = rectangle(3, 1);
  s.logical_area = s.visual_area;
  s.button = 0;

  if (strcmp(name, "3x1 Goban") == 0) {
    return single_valued(s, 2.5);
  }

  s.visual_area = rectangle(4, 1);
  s.logical_area = s.visual_area;

  if (strcmp(name, "4x1 Goban") == 0) {
    return single_valued(s, 3.5);
  }

  s.visual_area = rectangle(5, 1);
  s.logical_area = s.visual_area;

  if (strcmp(name, "5x1 Goban") == 0) {
    return dual_valued(s, -4.5, 5.5);
  }

  s.visual_area = rectangle(2, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "2x2 Goban") == 0) {
    return dual_valued(s, -3.5, 4.5);
  }

  s.visual_area = rectangle(3, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "3x2 Goban") == 0) {
    return dual_valued(s, -5.5, 6.5);
  }

  s.visual_area = rectangle(4, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "4x2 Goban") == 0) {
    return single_valued(s, 7.5);
  }


  // . . . . @
  // @ @ @ @ @
  s.visual_area = rectangle(5, 2);
  s.logical_area = rectangle(4, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Straight Four") == 0) {
    return single_valued(s, -9.5);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 998);
  }

  s.player = s.opponent;
  s.opponent = 0;

  if (strcmp(name, "Hat Four Defense") == 0) {
    return single_valued(s, 11.5);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 998);
  }

  s.ko_threats = -1;
  if (strcmp(name, "Bent Four in the Corner (1 ko threat)") == 0) {
    return single_valued(s, -5.5);
  }

  s.ko_threats = 0;
  s.logical_area ^= single(4, 0);
  s.player ^= single(4, 0);
  s.immortal = s.player;
  if (strcmp(name, "Bent Four in the Corner (1 liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 997.75);
  }

  s.logical_area ^= single(4, 1);
  s.player ^= single(4, 1);
  s.immortal = s.player;
  if (strcmp(name, "Bent Four in the Corner (2 liberties)") == 0) {
    return single_valued(s, -6.5);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE + 0.5, 998.25);
  }

  if (strcmp(name, "Bent Four in the Corner is Dead (defender has threats)") == 0) {
    s.ko_threats = -1;
    // The high value indicates that target loss can be delayed indefinitely
    return (tsumego) {s, -7.5625, 1000.4375, -7.5625, 72.9375};
  }

  if (strcmp(name, "Bent Four in the Corner is Dead (attacker tenuki)") == 0) {
    make_move(&s, pass());
    return delay_valued(s, -TARGET_CAPTURED_SCORE - 0.5, -998.5);;
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

  if (strcmp(name, "Bulky Five") == 0) {
    // Score: Target captured
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 997);
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

  if (strcmp(name, "Rectangle Six in the Corner") == 0) {
    return delay_valued(s, 999.4375, 996.9375);
  }

  s.logical_area ^= single(4, 0);
  s.immortal ^= single(4, 0);
  s.player ^= single(4, 0);
  s.ko_threats = 0;
  if (strcmp(name, "Rectangle Six in the Corner (1 physical liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 996.75);
  }

  s.player ^= single(4, 0);
  s.external ^= single(4, 0);

  if (strcmp(name, "Rectangle Six in the Corner (1 liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 996.75);
  }

  s.logical_area ^= single(4, 1);
  s.immortal ^= single(4, 1);
  s.external ^= single(4, 1);
  if (strcmp(name, "Rectangle Six in the Corner (2 liberties)") == 0) {
    return single_valued(s, -5.5);
  }


  if (strcmp(name, "Problem A") == 0) {
    s = parse_state("\
      , , B B B B B , , \
      B B B w w w w B , \
      . w w . . . w B , \
      . w . w . . w B , \
      B B w w . B B , , \
      , B B B B , , , , \
      , , , , , , , , , \
    ");
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 997.75);
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

  if (strcmp(name, "Rectangle Eight in the Corner") == 0) {
    return single_valued(s, -3.5);
  }

  s.ko_threats = -1;
  if (strcmp(name, "Rectangle Eight in the Corner (defender has threats)") == 0) {
    return single_valued(s, -11.5);
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

  if (strcmp(name, "Square Nine in the Corner") == 0) {
    return single_valued(s, -6.5);
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
    return single_valued(s, 0);
  }

  s.ko_threats = -2;
  if (strcmp(name, "Carpenter's Square (defender has threats)") == 0) {
    return single_valued(s, 0);
  }

  if (strcmp(name, "L Group") == 0) {
    s = parse_state("\
      . . . . . x x x x \
      W W b . . x x x x \
      , W b . . x x x x \
      , W b b . x x x x \
      W , W W . x x x x \
      , , , W . x x x x \
    ");

    return delay_valued(s, -TARGET_CAPTURED_SCORE + 0.5, -997.25);
  }

  s = parse_state("   \
    . . . . . . x x x \
    W W b b . . x x x \
    , , W b . . x x x \
    , , W b b . x x x \
    , W , W W . x x x \
    , , , , W . x x x \
  ");

  if (strcmp(name, "First L+1 Group Defense") == 0) {
    return single_valued(s, -6.5);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "First L+1 Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 997);
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

  if (strcmp(name, "Second L+1 Group Defense") == 0) {
    return single_valued(s, -5.5625);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = 0;
  if (strcmp(name, "Second L+1 Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 996.5);
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

  if (strcmp(name, "L+2 Group with Descent Defense") == 0) {
    return dual_valued(s, -3.5, -1.5);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = 1;
  if (strcmp(name, "L+2 Group with Descent Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 995.75);
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

  if (strcmp(name, "Basic J Group Defense") == 0) {
    return single_valued(s, -5.5);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "Basic J Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 997);
  }

  s.logical_area ^= single(0, 3) ^ single(0, 4);
  s.opponent ^= single(0, 3) ^ single(0, 4);
  s.target = s.opponent;
  if (strcmp(name, "Straight J Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 996.25);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "Straight J Group Defense") == 0) {
    return single_valued(s, -5.5);
  }

  s = parse_state("   \
    . . . . . . x x x \
    . . b b W W x x x \
    . . b W , , x x x \
    b . b W , , x x x \
    . b W , , , x x x \
    W W W , , , x x x \
  ");

  if (strcmp(name, "J+1 Group with Descent Defense") == 0) {
    return dual_valued(s, -2.5, -0.5);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = -1;
  if (strcmp(name, "J+1 Group with Descent Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - 0.5, 996.5);
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

  if (strcmp(name, "Long L Group Defense") == 0) {
    return single_valued(s, -4.5);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;

  if (strcmp(name, "Long L Group Attack") == 0) {
    return single_valued(s, 15.5);
  }

  s.ko_threats = 1;
  if (strcmp(name, "Long L Group Attack (with threats)") == 0) {
    // TODO
    return single_valued(s, 0);
  }

  fprintf(stderr, "Tsumego \"%s\" not found.\n", name);
  exit(EXIT_FAILURE);
}
