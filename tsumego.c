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
  "Bent Four Debug",
  "Bent Four in the Corner is Dead",
  "Bent Four in the Corner is Dead (defender has threats)",
  "Bent Four in the Corner is Dead (attacker tenuki)",
  "Bulky Five",
  "Rectangle Six in the Corner",
  "Rectangle Six in the Corner (1 physical liberty)",
  "Rectangle Six in the Corner (1 liberty)",
  "Rectangle Six in the Corner (2 liberties)",
  "Walkie Talkie Seven",
  "Walkie Talkie Seven (1 ko threat)",
  "Walkie Talkie Seven (3 liberties)",
  "Problem A",
  "Rectangle Eight in the Corner",
  "Rectangle Eight in the Corner (defender has threats)",
  "Square Nine in the Corner",
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
  "Escape A",
  "Six is Dead",
  "Seven on the Second Line Defense",
  "Seven on the Second Line Attack",
  "Eight is Alive",
  "Rabbity Six Defense",
  "Rabbity Six Attack",
  "Carpenter's Square",
  "Carpenter's Square (defender has threats)",
  "Carpenter's Square (1 liberty)"
};

const size_t NUM_TSUMEGO = sizeof(TSUMEGO_NAMES) / sizeof(char*);

tsumego single_valued(state s, float score) {
  return (tsumego) {s, score, score, score, score};
}

tsumego delay_valued(state s, float score, int delay) {
  return (tsumego) {
    s,
    score,
    score,
    score - delay * DELAY_BONUS,
    score - delay * DELAY_BONUS
  };
}

tsumego dual_valued(state s, float low, float high) {
  return (tsumego) {s, low, high, low, high};
}

// There's often tension between wasting ko-threats and delaying target capture
tsumego tension_valued(state s, float no_delay, float delay_base, int delay) {
  return (tsumego) {
    s,
    no_delay,
    no_delay,
    delay_base - delay * DELAY_BONUS,
    delay_base - delay * DELAY_BONUS
  };
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
  s.wide = false;

  if (strcmp(name, "Straight Three") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 3);
  }

  s.player = s.opponent;
  s.opponent = 0;

  if (strcmp(name, "Straight Three Defense") == 0) {
    return single_valued(s, 8 - BUTTON_BONUS);
  }

  // . . @
  // @ @ @
  s.visual_area = rectangle(3, 2);
  s.logical_area = rectangle(2, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Straight Two") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE + BUTTON_BONUS, 2);
  }

  // . .
  s.visual_area = rectangle(2, 1);
  s.logical_area = rectangle(2, 1);
  s.player = 0;
  s.opponent = 0;
  s.target = 0;

  if (strcmp(name, "2x1 Goban") == 0) {
    return dual_valued(s, -2 + BUTTON_BONUS, 2 + BUTTON_BONUS);
  }

  s.button = 1;
  if (strcmp(name, "2x1 Goban (lost button)") == 0) {
    // Score: -1.5 to 2.5
    return dual_valued(s, -2 + BUTTON_BONUS, 2 + BUTTON_BONUS);
  }

  s.visual_area = rectangle(3, 1);
  s.logical_area = s.visual_area;
  s.button = 0;

  if (strcmp(name, "3x1 Goban") == 0) {
    return single_valued(s, 3 - BUTTON_BONUS);
  }

  s.visual_area = rectangle(4, 1);
  s.logical_area = s.visual_area;

  if (strcmp(name, "4x1 Goban") == 0) {
    return single_valued(s, 4 - BUTTON_BONUS);
  }

  s.visual_area = rectangle(5, 1);
  s.logical_area = s.visual_area;

  if (strcmp(name, "5x1 Goban") == 0) {
    return dual_valued(s, -5 + BUTTON_BONUS, 5 + BUTTON_BONUS);
  }

  s.visual_area = rectangle(2, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "2x2 Goban") == 0) {
    return dual_valued(s, -4 + BUTTON_BONUS, 4 + BUTTON_BONUS);
  }

  s.visual_area = rectangle(3, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "3x2 Goban") == 0) {
    return dual_valued(s, -6 + BUTTON_BONUS, 6 + BUTTON_BONUS);
  }

  s.visual_area = rectangle(4, 2);
  s.logical_area = s.visual_area;

  if (strcmp(name, "4x2 Goban") == 0) {
    return single_valued(s, 8 - BUTTON_BONUS);
  }


  // . . . . @
  // @ @ @ @ @
  s.visual_area = rectangle(5, 2);
  s.logical_area = rectangle(4, 1);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;

  if (strcmp(name, "Straight Four") == 0) {
    return single_valued(s, -10 + BUTTON_BONUS);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 6);
  }

  s.player = s.opponent;
  s.opponent = 0;

  if (strcmp(name, "Hat Four Defense") == 0) {
    return single_valued(s, 12 - BUTTON_BONUS);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 6);
  }

  s.ko_threats = -1;
  if (strcmp(name, "Bent Four in the Corner (1 ko threat)") == 0) {
    return single_valued(s, -6 + BUTTON_BONUS);
  }

  s.ko_threats = 0;
  s.logical_area ^= single(4, 0);
  s.player ^= single(4, 0);
  s.immortal = s.player;
  if (strcmp(name, "Bent Four in the Corner (1 liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 7);
  }

  s.logical_area ^= single(4, 1);
  s.player ^= single(4, 1);
  s.immortal = s.player;
  if (strcmp(name, "Bent Four in the Corner (2 liberties)") == 0) {
    return single_valued(s, -6 - BUTTON_BONUS);
  }

  s.external |= single(4, 0) | single(4, 1);
  s.player |= s.external | single(1, 0);
  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.white_to_play = true;
  if (strcmp(name, "Bent Four Debug") == 0) {
    return single_valued(s, 6 - BUTTON_BONUS);
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
  s.external = 0;
  s.target = s.opponent;

  if (strcmp(name, "Bent Four in the Corner is Dead") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE + BUTTON_BONUS, 9);
  }

  if (strcmp(name, "Bent Four in the Corner is Dead (defender has threats)") == 0) {
    s.ko_threats = -1;
    // The high value indicates that target loss can be delayed indefinitely
    return (tsumego) {
        s,
        -8 + BUTTON_BONUS - KO_THREAT_BONUS,
        TARGET_CAPTURED_SCORE + BUTTON_BONUS - KO_THREAT_BONUS,
        -8 + BUTTON_BONUS - KO_THREAT_BONUS,
        72.71875
    };
  }

  if (strcmp(name, "Bent Four in the Corner is Dead (attacker tenuki)") == 0) {
    make_move(&s, pass());
    return delay_valued(s, -TARGET_CAPTURED_SCORE - BUTTON_BONUS, -8);;
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 10);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS - KO_THREAT_BONUS, 10);
  }

  s.logical_area ^= single(4, 0);
  s.immortal ^= single(4, 0);
  s.player ^= single(4, 0);
  s.ko_threats = 0;
  if (strcmp(name, "Rectangle Six in the Corner (1 physical liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 11);
  }

  s.player ^= single(4, 0);
  s.external ^= single(4, 0);

  if (strcmp(name, "Rectangle Six in the Corner (1 liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 11);
  }

  s.logical_area ^= single(4, 1);
  s.immortal ^= single(4, 1);
  s.external ^= single(4, 1);
  if (strcmp(name, "Rectangle Six in the Corner (2 liberties)") == 0) {
    return single_valued(s, -6 + BUTTON_BONUS);
  }

  // . . . . @
  // . . . @ @
  // @ @ @ @ @

  s.visual_area = rectangle(5, 3);
  s.logical_area = rectangle(3, 2) | single(3, 0);
  s.player = 0;
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;
  s.immortal = 0;
  s.external = 0;
  s.ko_threats = 0;

  if (strcmp(name, "Walkie Talkie Seven") == 0) {
    return single_valued(s, -5 - BUTTON_BONUS);
  }

  s.ko_threats = 1;
  if (strcmp(name, "Walkie Talkie Seven (1 ko threat)") == 0) {
    return tension_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, TARGET_CAPTURED_SCORE - BUTTON_BONUS + KO_THREAT_BONUS, 11);
  }

  s.external = rectangle(1, 3) << 5;
  s.visual_area |= s.external;
  s.logical_area |= s.external;
  s.player |= s.external;
  if (strcmp(name, "Walkie Talkie Seven (3 liberties)") == 0) {
    return single_valued(s, -12 + BUTTON_BONUS + KO_THREAT_BONUS);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 7);
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
    return single_valued(s, -4 + BUTTON_BONUS);
  }

  s.ko_threats = -1;
  if (strcmp(name, "Rectangle Eight in the Corner (defender has threats)") == 0) {
    return single_valued(s, -12 + BUTTON_BONUS);
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
    return single_valued(s, -7 + BUTTON_BONUS);
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
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 11);
  }

  s.ko_threats = -1;
  if (strcmp(name, "Carpenter's Square (defender has threats)") == 0) {
    return single_valued(s, 14 - BUTTON_BONUS - KO_THREAT_BONUS);
  }

  s.ko_threats = 0;
  s.external |= single(4, 2);
  s.immortal ^= s.external;
  s.logical_area |= s.external;
  if (strcmp(name, "Carpenter's Square (1 liberty)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 19);
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

    return delay_valued(s, -TARGET_CAPTURED_SCORE + BUTTON_BONUS, -12);
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
    return single_valued(s, -7 + BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "First L+1 Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 12);
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
    // 5225437 nodes expanded (4x3 tablebase)
    // -7.781250*, -7.781250*
    // 5037354 nodes expanded (4x3 tablebase)
    // -6.250000*, -5.781250*
    return single_valued(s, -6 + BUTTON_BONUS - KO_THREAT_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = 0;
  if (strcmp(name, "Second L+1 Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 13);
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
    return dual_valued(s, -4 + BUTTON_BONUS, -2 + BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = 1;
  if (strcmp(name, "L+2 Group with Descent Attack") == 0) {
    return tension_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, TARGET_CAPTURED_SCORE - BUTTON_BONUS + KO_THREAT_BONUS, 17);
  }

  s = parse_state("   \
    . . . . . x x x x \
    . . b W W x x x x \
    . . b W , x x x x \
    @ . b W , x x x x \
    . @ W , , x x x x \
    . W W , , x x x x \
    . W , W , x x x x \
  ");

  if (strcmp(name, "Basic J Group Defense") == 0) {
    return single_valued(s, -5 - BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "Basic J Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 12);
  }

  s.opponent ^= single(0, 3) ^ single(0, 4);
  if (strcmp(name, "Straight J Group Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 17);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  if (strcmp(name, "Straight J Group Defense") == 0) {
    return single_valued(s, -5 - BUTTON_BONUS);
  }

  s = parse_state("   \
    . . . . . . x x x \
    . . b b W W x x x \
    . . b W , , x x x \
    @ . b W , , x x x \
    . @ W , , , x x x \
    W W W , , , x x x \
  ");

  if (strcmp(name, "J+1 Group with Descent Defense") == 0) {
    return dual_valued(s, -3 + BUTTON_BONUS, -1 + BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.ko_threats = -1;
  if (strcmp(name, "J+1 Group with Descent Attack") == 0) {
    return tension_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS - KO_THREAT_BONUS, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 16);
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
    return single_valued(s, -4 - BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;

  if (strcmp(name, "Long L Group Attack") == 0) {
    return single_valued(s, 16 - BUTTON_BONUS);
  }

  s.ko_threats = 1;
  if (strcmp(name, "Long L Group Attack (with threats)") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS + KO_THREAT_BONUS, 20);
  }

  s = parse_state("   \
    . @ . w . 0 . . . \
    w w w w w B . . W \
    + + B B B B , , , \
  ");
  s.ko_threats = -2;

  if (strcmp(name, "Escape A") == 0) {
    return tension_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS - 2 * KO_THREAT_BONUS, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 12);
  }

  s = parse_state("                 \
    . . . . . . . . . . x x x x x x \
    W W b b b b b b W W x x x x x x \
    , W W W W W W W W , x x x x x x \
  ");
  s.ko_threats = 1;
  s.wide = true;

  if (strcmp(name, "Six is Dead") == 0) {
    return delay_valued(s, -TARGET_CAPTURED_SCORE + BUTTON_BONUS + KO_THREAT_BONUS, -8);
  }

  s = parse_state("                 \
    . . . . . . . . . . . x x x x x \
    W W b b b b b b b W W x x x x x \
    , W W W W W W W W , , x x x x x \
  ");
  s.wide = true;

  if (strcmp(name, "Seven on the Second Line Defense") == 0) {
    return single_valued(s, -5 + BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;

  if (strcmp(name, "Seven on the Second Line Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 10);
  }

  s = parse_state("                 \
    . . . . . . . . . . . . x x x x \
    B B w w w w w w w w B B x x x x \
    , , B B B B B B B B , B x x x x \
    , B , , , , , , , , , B x x x x \
  ");
  s.wide = true;
  s.ko_threats = 1;

  if (strcmp(name, "Eight is Alive") == 0) {
    return single_valued(s, 16 + BUTTON_BONUS + KO_THREAT_BONUS);
  }

  s = parse_state("\
    x x x b . . b b xxxx xxxx \
    x x x b . . . b xxxx xxxx \
    x x x b b . b b xxxx xxxx \
    x x x b b b b b xxxx xxxx \
  ");
  s.wide = true;

  if (strcmp(name, "Rabbity Six Defense") == 0) {
    return single_valued(s, 20 - BUTTON_BONUS);
  }

  temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;

  if (strcmp(name, "Rabbity Six Attack") == 0) {
    return delay_valued(s, TARGET_CAPTURED_SCORE - BUTTON_BONUS, 15);
  }

  s = parse_state("\
    W W b 0 . 0 b - W W x x x x x x \
    W W b b b b b b W W x x x x x x \
    , W W W W W W W W , x x x x x x \
  ");
  s.wide = true;

  if (strcmp(name, "Edge Debug") == 0) {
    return single_valued(s, -8 + BUTTON_BONUS);
  }

  fprintf(stderr, "Tsumego \"%s\" not found.\n", name);
  exit(EXIT_FAILURE);
}
