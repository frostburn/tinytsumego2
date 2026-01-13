#include <stdio.h>
#include "tinytsumego2/state.h"

void print_state(const state *s) {
  stones_t black, white;
  if (s->white_to_play) {
    white = s->player;
    black = s->opponent;
  }
  else {
    black = s->player;
    white = s->opponent;
  }

  // Column headers
  printf(" ");
  for (int i = 0; i < WIDTH; i++) {
      printf(" %c", 'A' + i);
  }
  printf("\n");

  for (int i = 0; i < STATE_SIZE; i++) {
    // Row headers
    if (i % V_SHIFT == 0) {
      printf("%d", i / V_SHIFT);
    }

    stones_t p = (1ULL << i);

    // Visual / logical playing area indicators
    if (p & s->visual_area) {
      if (p & s->logical_area) {
        printf("\x1b[0;30;43m");  // Yellow BG
      } else {
        printf("\x1b[0;30;46m");  // Cyan BG
      }
    }
    else {
      if (p & s->logical_area) {
        printf("\x1b[0;30;101m");  // Bright Red BG (error)
      } else {
        printf("\x1b[0m");  // No BG (outside)
      }
    }

    // Stones
    if (p & black) {
      printf("\x1b[30m");  // Black
      if (p & s->target) {
        printf(" b");
      }
      else if (p & s->immortal) {
        printf(" B");
      } else if (p & s->external) {
        printf(" +");
      }
      else if (p & white) {
        printf(" #");
      } else {
        printf(" @");
      }
    }
    else if (p & white) {
      printf("\x1b[97m");  // Bright White
      if (p & s->target) {
        printf(" w");
      }
      else if (p & s->immortal) {
        printf(" W");
      } else if (p & s->external) {
        printf(" -");
      }
      else {
        printf(" 0");
      }
    }
    else if (p & s->ko) {
      printf("\x1b[35m");
      printf(" *");
    }
    else if (p & s->visual_area) {
      if (p & s->target) {
        printf("\x1b[34m");
        printf(" !");
      } else {
        printf("\x1b[35m");
        if (p & s->logical_area) {
          printf(" .");
        } else {
          printf(" ,");
        }
      }
    }
    else if (p & s->logical_area) {
      printf("\x1b[35m");
      printf(" ?");
    }
    else {
        printf("  ");
    }
    if (i % V_SHIFT == V_SHIFT - 1){
        printf("\x1b[0m\n");
    }
  }
  printf("passes = %d ko_threats = %d button = %d\n", s->passes, s->ko_threats, s->button);
  if (s->white_to_play) {
    printf("White to play\n");
  }
  else {
    printf("Black to play\n");
  }
}

void repr_state(const state *s) {
    printf(
        "(state) {%lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %d, %d, %d, %s}\n",
        s->visual_area,
        s->logical_area,
        s->player,
        s->opponent,
        s->ko,
        s->target,
        s->immortal,
        s->external,
        s->passes,
        s->ko_threats,
        s->button,
        s->white_to_play ? "true" : "false"
    );
}

state parse_state(const char *visuals) {
  state s;
  s.visual_area = 0;
  s.logical_area = 0;
  s.player = 0;
  s.opponent = 0;
  s.ko = 0;
  s.target = 0;
  s.immortal = 0;
  s.external = 0;
  s.passes = 0;
  s.ko_threats = 0;
  s.button = 0;
  s.white_to_play = false;

  stones_t p = 1ULL;
  while (*visuals) {
    switch (*visuals) {
      case 'x':
        p <<= 1;
        break;
      case '.':
        s.visual_area |= p;
        s.logical_area |= p;
        p <<= 1;
        break;
      case ',':
        s.visual_area |= p;
        p <<= 1;
        break;
      case '*':
        s.visual_area |= p;
        s.logical_area |= p;
        s.ko |= p;
        p <<= 1;
        break;
      case '@':
        s.visual_area |= p;
        s.logical_area |= p;
        s.player |= p;
        p <<= 1;
        break;
      case 'b':
        s.visual_area |= p;
        s.player |= p;
        s.target |= p;
        p <<= 1;
        break;
      case 'B':
        s.visual_area |= p;
        s.player |= p;
        s.immortal |= p;
        p <<= 1;
        break;
      case '+':
        s.visual_area |= p;
        s.logical_area |= p;
        s.player |= p;
        s.external |= p;
        p <<= 1;
        break;
      case '0':
        s.visual_area |= p;
        s.logical_area |= p;
        s.opponent |= p;
        p <<= 1;
        break;
      case 'w':
        s.visual_area |= p;
        s.opponent |= p;
        s.target |= p;
        p <<= 1;
        break;
      case 'W':
        s.visual_area |= p;
        s.opponent |= p;
        s.immortal |= p;
        p <<= 1;
        break;
      case '-':
        s.visual_area |= p;
        s.logical_area |= p;
        s.opponent |= p;
        s.external |= p;
        p <<= 1;
        break;
    }
    visuals++;
  }
  return s;
}

move_result make_move(state *s, stones_t move) {
  move_result result = NORMAL;
  stones_t old_player = s->player;
  // Handle pass
  if (!move) {
    // Award button if still available
    if (s->button == 0) {
      s->button = 1;
      result = TAKE_BUTTON;
    }
    // Clear ko w/o incrementing passes
    if (s->ko){
      s->ko = 0;
      result = CLEAR_KO;
    }
    // Only count regular passes
    else if (result != TAKE_BUTTON) {
      s->passes++;
      if (s->passes == 1) {
        result = PASS;
      } else {
        result = SECOND_PASS;
      }
    }
    // Swap players
    s->player = s->opponent;
    s->opponent = old_player;
    s->ko_threats = -s->ko_threats;
    s->button = -s->button;
    s->white_to_play = !s->white_to_play;
    return result;
  }

  // Handle regular move
  stones_t old_opponent = s->opponent;
  stones_t old_ko = s->ko;
  int old_ko_threats = s->ko_threats;
  if (move & s->ko) {
    // Illegal ko move
    if (s->ko_threats <= 0) {
      return ILLEGAL;
    }
    // Legal ko move by playing an external threat first
    s->ko_threats--;
    result = KO_THREAT_AND_RETAKE;
  }

  // Abort if move outside empty logical area
  stones_t empty = s->logical_area & ~(s->player ^ s->external) & ~s->opponent;
  if (move & ~empty) {
    return ILLEGAL;
  }

  if (move & s->external) {
    // Normalize move placement inside the group of external liberties.
    move = 1ULL << ctz(flood(move, s->external));
    s->immortal |= move;
    s->external ^= move;
    result = FILL_EXTERNAL;
  }

  s->player |= move;
  s->ko = 0;

  // Opponent's stones killed
  stones_t kill = 0;

  // Potential liberties for opponent's stones (visual non-logical liberties count as permanent)
  empty = s->visual_area & ~(s->player ^ s->external);

  // Lol, macro abuse
  #define KILL_CHAIN \
  if (!liberties(chain, empty) && !(chain & s->immortal)) {\
    kill |= chain;\
    s->opponent ^= chain;\
  }
  stones_t chain = flood(move >> V_SHIFT, s->opponent);
  KILL_CHAIN
  chain = flood(move << V_SHIFT, s->opponent);
  KILL_CHAIN
  chain = flood((move >> H_SHIFT) & WEST_BLOCK, s->opponent);
  KILL_CHAIN
  chain = flood((move & WEST_BLOCK) << H_SHIFT, s->opponent);
  KILL_CHAIN

  // Check legality
  chain = flood(move, s->player & ~s->external);
  if (!liberties(chain, s->visual_area & ~(s->opponent ^ s->external)) && !(chain & s->immortal)) {
    // Oops! Revert state
    s->player = old_player;
    s->opponent = old_opponent;
    s->ko = old_ko;
    s->ko_threats = old_ko_threats;
    return ILLEGAL;
  }

  // Bit magic to check if a single stone was killed and the played stone was left alone in atari
  if (
    (kill & (kill - 1ULL)) == 0ULL &&
    (chain & (chain - 1ULL)) == 0ULL &&
    liberties(chain, s->logical_area & ~s->opponent) == kill
   ) {
    s->ko = kill;
  }

  // Expand immortal areas
  if (chain & s->immortal) {
    s->immortal |= chain;
  }

  // Expand target areas
  if (chain & s->target) {
    s->target |= chain;
  }

  // Swap players
  s->passes = 0;
  old_player = s->player;
  s->player = s->opponent;
  s->opponent = old_player;
  s->ko_threats = -s->ko_threats;
  s->button = -s->button;
  s->white_to_play = !s->white_to_play;

  if (kill & s->target) {
    return TAKE_TARGET;
  }

  return result;
}

size_t to_key(state *root, state *child) {
  size_t key = 0;
  int j;

  key = key * 2 + !!child->white_to_play;
  key = key * 3 + child->button + 1;
  key = key * (2 * abs(root->ko_threats) + 1) + child->ko_threats + abs(root->ko_threats);
  key = key * 2 + child->passes;

  stones_t effective_area = root->logical_area & ~(root->target | root->immortal);

  // Index ko
  key *= popcount(effective_area) + 1;
  j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (p & root->logical_area) {
      j++;
      if (p & child->ko) {
        key += j;
        break;
      }
    }
  }

  // Encode stones in ternary
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (p & effective_area) {
      key *= 3;
      if (p & child->player) {
        key += 1;
      } else if (p & child->opponent) {
        key += 2;
      }
    }
  }

  // TODO: Make external liberty counting more lightweight

  return key;
}

size_t keyspace_size(state *root) {
  const size_t num_moves = popcount(root->logical_area & ~(root->target | root->immortal));
  size_t result = (
    2 * // Player to play
    3 * // Button ownership
    (2 * abs(root->ko_threats) + 1) * // Number and ownership of the remaining "external" ko threats
    2 * // Passes
    (num_moves + 1) // Location (or absence) of the illegal ko square
  );
  // Ternary encoding of stones
  for (size_t i = 0; i < num_moves; ++i) {
    result *= 3;
  }
  return result;
}

int chinese_liberty_score(const state *s) {
  stones_t player_controlled = s->player | liberties(s->player, s->visual_area & ~s->opponent);
  stones_t opponent_controlled = s->opponent | liberties(s->opponent, s->visual_area & ~s->player);
  return popcount(player_controlled) - popcount(opponent_controlled);
}

bool equals(const state *a, const state *b) {
  if (a->visual_area != b->visual_area) {
    return false;
  }
  if (a->logical_area != b->logical_area) {
    return false;
  }
  if (a->player != b->player) {
    return false;
  }
  if (a->opponent != b->opponent) {
    return false;
  }
  if (a->ko != b->ko) {
    return false;
  }
  if (a->target != b->target) {
    return false;
  }
  if (a->immortal != b->immortal) {
    return false;
  }
  if (a->external != b->external) {
    return false;
  }
  if (a->passes != b->passes) {
    return false;
  }
  if (a->ko_threats != b->ko_threats) {
    return false;
  }
  if (a->button != b->button) {
    return false;
  }
  if (a->white_to_play != b->white_to_play) {
    return false;
  }
  return true;
}

int compare(const void *a_, const void *b_) {
    state *a = (state*) a_;
    state *b = (state*) b_;
    if (a->player < b->player) {
        return -1;
    }
    if (a->player > b->player) {
        return 1;
    }

    if (a->opponent < b->opponent) {
        return -1;
    }
    if (a->opponent > b->opponent) {
        return 1;
    }

    if (a->ko < b->ko) {
        return -1;
    }
    if (a->ko > b->ko) {
        return 1;
    }

    if (a->external < b->external) {
        return -1;
    }
    if (a->external > b->external) {
        return 1;
    }

    if (a->passes < b->passes) {
        return -1;
    }
    if (a->passes > b->passes) {
        return 1;
    }

    if (a->ko_threats < b->ko_threats) {
        return -1;
    }
    if (a->ko_threats > b->ko_threats) {
        return 1;
    }

    if (a->button < b->button) {
        return -1;
    }
    if (a->button > b->button) {
        return 1;
    }

    if (a->white_to_play < b->white_to_play) {
        return -1;
    }
    if (a->white_to_play > b->white_to_play) {
        return 1;
    }

    return 0;
}

stones_t hash_a(const state *s) {
  stones_t result = s->passes;
  result = (result << 4) | s->ko_threats;
  result = (result << 4) | s->button;
  result = (result << 4) | s->white_to_play;
  result ^= s->ko * 98765ULL;
  result ^= s->external * 12345ULL;
  result ^= s->player;
  result *= 7777771234563346789ULL;
  result ^= (s->opponent << 32) | (s->opponent >> 32);
  return result;
}

stones_t hash_b(const state *s) {
  stones_t result = s->passes;
  result = (result << 5) | s->ko_threats;
  result = (result << 5) | s->button;
  result = (result << 5) | s->white_to_play;
  result *= 582327ULL;
  result ^= s->ko;
  result ^= s->external * 23345ULL;
  result *= 12356743289ULL;
  result ^= (s->player << 32) | (s->player >> 32);
  result ^= s->opponent;
  return result * 1238767834675843ULL;
}

stones_t benson(stones_t visual_area, stones_t black, stones_t white, stones_t immortal) {
  int num_chains;
  stones_t *black_chains = chains(black & ~immortal, &num_chains);
  stones_t black_enclosed = visual_area ^ black;
  int num_regions;
  stones_t *regions = chains(black_enclosed, &num_regions);
  stones_t white_mortal = white & ~immortal;

  bool **vital = malloc(num_chains * sizeof(bool*));
  bool **adjacent = malloc(num_chains * sizeof(bool*));
  for (int i = 0; i < num_chains; ++i) {
    vital[i] = calloc(num_regions, sizeof(bool));
    adjacent[i] = calloc(num_regions, sizeof(bool));
  }

  for (int i = 0; i < num_chains; ++i) {
    stones_t libs = cross(black_chains[i]);
    for (int j = 0; j < num_regions; ++j) {
      if (!(regions[j] & ~white_mortal & ~libs)) {
        vital[i][j] = true;
        adjacent[i][j] = true;
      } else if (cross(regions[j]) & black_chains[i]) {
        adjacent[i][j] = true;
      }
    }
  }

  bool done = false;

  while (!done) {
    done = true;
    for (int i = 0; i < num_chains; ++i) {
      int num_vital = 0;
      for (int j = 0; j < num_regions; ++j) {
        if (vital[i][j]) {
          num_vital++;
        }
      }
      if (num_vital < 2 && black_chains[i]) {
        done = false;
        black_chains[i] = 0ULL;
        for (int j = 0; j < num_regions; ++j) {
          if (adjacent[i][j]) {
            for (int k = 0; k < num_chains; ++k) {
              vital[k][j] = false;
            }
            regions[j] = 0ULL;
          }
        }
      }
    }
  }

  for (int j = 0; j < num_regions; ++j) {
    int num_vital = 0;
    for (int i = 0; i < num_chains; ++i) {
      if (vital[i][j]) {
        num_vital++;
      }
    }
    if (!num_vital) {
      regions[j] = 0ULL;
    }
  }

  stones_t result = 0ULL;
  for (int i = 0; i < num_chains; ++i) {
    result |= black_chains[i];
  }
  for (int j = 0; j < num_regions; ++j) {
    result |= regions[j];
  }

  free(black_chains);
  free(regions);
  free(vital);

  return result;
}

move_result apply_benson(state *s) {
  move_result result = NORMAL;

  stones_t ext_mask = ~s->external;
  stones_t player_unconditional = benson(s->visual_area, s->player & ext_mask, s->opponent & ext_mask, s->immortal);
  // Convert dead opponent stones (Makes no difference under Chinese rules)
  // TODO: Consider capturing instead for less confusion
  stones_t eyespace = s->opponent & player_unconditional;
  if (eyespace & s->target) {
    result = TARGET_LOST;
  }
  s->player |= eyespace;
  s->opponent ^= eyespace;
  // Visualize unconditionally alive stones as immortal
  s->immortal |= s->player & player_unconditional;

  stones_t opponent_unconditional = benson(s->visual_area, s->opponent & ext_mask, s->player & ext_mask, s->immortal);
  eyespace = s->player & opponent_unconditional;
  if (eyespace & s->target) {
    result = TAKE_TARGET;
  }
  s->opponent |= eyespace;
  s->player ^= eyespace;
  s->immortal |= s->opponent & opponent_unconditional;

  // Remove unconditionally alive stones and their eye-space from consideration
  s->logical_area ^= s->logical_area & (player_unconditional | opponent_unconditional);

  return result;
}
