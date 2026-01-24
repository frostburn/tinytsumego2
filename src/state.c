#include <stdint.h>
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

  int width = s->wide ? WIDTH_16 : WIDTH;
  int height = s->wide ? HEIGHT_16 : HEIGHT;

  // Column headers
  printf(" ");
  for (int i = 0; i < width; i++) {
      printf(" %c", 'A' + i);
  }
  printf("\n");

  for (int i = 0; i < width * height; i++) {
    // Row headers
    if (i % width == 0) {
      printf("%d", i / width);
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
        if (p & s->immortal) {
          printf("\x1b[1m");  // Bold
        }
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
        if (p & s->immortal) {
          printf("\x1b[1m");  // Bold
        }
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
    if (i % width == width - 1){
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
        "(state) {%lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %lluULL, %d, %d, %d, %s, %s}\n",
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
        s->white_to_play ? "true" : "false",
        s->wide ? "true" : "false"
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
    if (s->wide) {
      move = 1ULL << ctz(flood_16(move, s->external));
    } else {
      move = 1ULL << ctz(flood(move, s->external));
    }
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

  stones_t chain;
  stones_t libs;

  if (s->wide) {
    // Lol, macro abuse
    #define KILL_CHAIN_16 \
    if (!liberties_16(chain, empty) && !(chain & s->immortal)) {\
      kill |= chain;\
      s->opponent ^= chain;\
    }
    chain = flood_16(move >> V_SHIFT_16, s->opponent);
    KILL_CHAIN_16
    chain = flood_16(move << V_SHIFT_16, s->opponent);
    KILL_CHAIN_16
    chain = flood_16((move >> H_SHIFT_16) & WEST_BLOCK_16, s->opponent);
    KILL_CHAIN_16
    chain = flood_16((move & WEST_BLOCK_16) << H_SHIFT_16, s->opponent);
    KILL_CHAIN_16

    // Check legality
    chain = flood_16(move, s->player & ~s->external);
    libs = liberties_16(chain, s->visual_area & ~(s->opponent ^ s->external));
  } else {
    // Lol, macro abuse
    #define KILL_CHAIN \
    if (!liberties(chain, empty) && !(chain & s->immortal)) {\
      kill |= chain;\
      s->opponent ^= chain;\
    }
    chain = flood(move >> V_SHIFT, s->opponent);
    KILL_CHAIN
    chain = flood(move << V_SHIFT, s->opponent);
    KILL_CHAIN
    chain = flood((move >> H_SHIFT) & WEST_BLOCK, s->opponent);
    KILL_CHAIN
    chain = flood((move & WEST_BLOCK) << H_SHIFT, s->opponent);
    KILL_CHAIN

    // Check legality
    chain = flood(move, s->player & ~s->external);
    libs = liberties(chain, s->visual_area & ~(s->opponent ^ s->external));
  }
  if (!libs && !(chain & s->immortal)) {
    // Oops! Revert state
    s->player = old_player;
    s->opponent = old_opponent;
    s->ko = old_ko;
    s->ko_threats = old_ko_threats;
    return ILLEGAL;
  }

  // Bit magic to check if a single stone was killed and the played stone was left alone in atari
  if (s->wide) {
    libs = liberties_16(chain, s->logical_area & ~s->opponent);
  } else {
    libs = liberties(chain, s->logical_area & ~s->opponent);
  }
  if (
    (kill & (kill - 1ULL)) == 0ULL &&
    (chain & (chain - 1ULL)) == 0ULL &&
    libs == kill
   ) {
    s->ko = kill;
  }

  // Expand immortal areas
  if (chain & s->immortal) {
    s->immortal |= chain;
    s->logical_area &= ~chain;
  }

  // Expand target areas
  if (chain & s->target) {
    s->target |= chain;
    s->logical_area &= ~chain;
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

  stones_t effective_area = root->logical_area & ~(root->target | root->immortal | root->external);

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

  // For simplicity we assume that external liberties belonging to a given player form a contiguous chain
  key = key * (popcount(root->external & root->player) + 1) + popcount(child->external & child->player);
  key = key * (popcount(root->external & root->opponent) + 1) + popcount(child->external & child->opponent);

  return key;
}

size_t keyspace_size(state *root) {
  const size_t num_moves = popcount(root->logical_area & ~(root->target | root->immortal | root->external));
  size_t result = (
    2 * // Player to play
    3 * // Button ownership
    (2 * abs(root->ko_threats) + 1) * // Number and ownership of the remaining "external" ko threats
    2 * // Passes
    (num_moves + 1) * // Location (or absence) of the illegal ko square
    (popcount(root->external & root->player) + 1) * // Number of external liberties filled by the player
    (popcount(root->external & root->opponent) + 1) // Number of external liberties filled by the opponent
  );
  // Ternary encoding of stones
  for (size_t i = 0; i < num_moves; ++i) {
    if (result >= SIZE_MAX / 3) {
      fprintf(stderr, "Warning: Keyspace overflow\n");
    }
    result *= 3;
  }
  return result;
}

int compare_keys(const void *a_, const void *b_) {
  size_t a = *((size_t*) a_);
  size_t b = *((size_t*) b_);
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}

int chinese_liberty_score(const state *s) {
  stones_t empty = (s->visual_area & ~(s->player | s->opponent)) | s->external;

  stones_t player_controlled = s->player & ~s->external;
  stones_t opponent_controlled = s->opponent & ~s->external;

  if (s->wide) {
    player_controlled |= liberties_16(player_controlled, empty);
    opponent_controlled |= liberties_16(opponent_controlled, empty);
  } else {
    player_controlled |= liberties(player_controlled, empty);
    opponent_controlled |= liberties(opponent_controlled, empty);
  }

  return popcount(player_controlled) - popcount(opponent_controlled);
}

int compensated_liberty_score(const state *s) {
  stones_t empty = s->visual_area & ~(s->player | s->opponent);

  stones_t player_controlled = s->player;
  stones_t opponent_controlled = s->opponent;

  if (s->wide) {
    player_controlled |= liberties_16(s->player, empty);
    opponent_controlled |= liberties_16(s->opponent, empty);
  } else {
    player_controlled |= liberties(s->player, empty);
    opponent_controlled |= liberties(s->opponent, empty);
  }

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
  if (a->wide != b->wide) {
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

stones_t benson(stones_t visual_area, stones_t black, stones_t white, stones_t immortal, bool wide) {
  int num_chains;
  stones_t *black_chains;
  if (wide) {
    black_chains = chains_16(black & ~immortal, &num_chains);
  } else {
    black_chains = chains(black & ~immortal, &num_chains);
  }
  stones_t black_enclosed = visual_area ^ black;
  int num_regions;
  stones_t *regions;
  if (wide) {
    regions = chains_16(black_enclosed, &num_regions);
  } else {
    regions = chains(black_enclosed, &num_regions);
  }
  stones_t white_mortal = white & ~immortal;
  stones_t white_immortal = white & immortal;
  stones_t black_cross = wide ? cross_16(black) : cross(black);
  stones_t non_liberties = visual_area & ~black_cross & ~(white & (wide ? cross_16(black_cross) : cross(black_cross)));

  int i = 0;
  for (int j = 0; j < num_regions; ++j) {
    // Immortal stones poison a region
    if (regions[j] & white_immortal) {
      continue;
    }
    // You could technically live under the stones of a large region
    // The bigger reason is that captured large chains don't give points under liberty scoring
    if (regions[j] & non_liberties) {
      continue;
    }
    regions[i++] = regions[j];
  }
  num_regions = i;

  bool **vital = malloc(num_chains * sizeof(bool*));
  bool **adjacent = malloc(num_chains * sizeof(bool*));
  for (int i = 0; i < num_chains; ++i) {
    vital[i] = calloc(num_regions, sizeof(bool));
    adjacent[i] = calloc(num_regions, sizeof(bool));
  }

  for (int i = 0; i < num_chains; ++i) {
    stones_t libs = wide ? cross_16(black_chains[i]) : cross(black_chains[i]);
    for (int j = 0; j < num_regions; ++j) {
      if (!(regions[j] & ~white_mortal & ~libs)) {
        vital[i][j] = true;
        adjacent[i][j] = true;
      } else if ((wide ? cross_16(regions[j]) : cross(regions[j])) & black_chains[i]) {
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

  for (int i = 0; i < num_chains; ++i) {
    free(vital[i]);
    free(adjacent[i]);
  }

  free(black_chains);
  free(regions);
  free(vital);
  free(adjacent);

  return result;
}

move_result apply_benson(state *s) {
  move_result result = NORMAL;

  stones_t ext_mask = ~s->external;
  stones_t player_unconditional = benson(s->visual_area, s->player & ext_mask, s->opponent & ext_mask, s->immortal, s->wide);
  // Capture dead opponent stones (Makes no difference under Chinese rules)
  stones_t dead = s->opponent & player_unconditional;
  if (dead & s->target) {
    result = TARGET_LOST;
  }
  if (s->wide) {
    s->player |= liberties_16(dead, player_unconditional);
  } else {
    s->player |= liberties(dead, player_unconditional);
  }
  s->opponent ^= dead;
  // Visualize unconditionally alive stones as immortal
  s->immortal |= s->player & player_unconditional;

  stones_t opponent_unconditional = benson(s->visual_area, s->opponent & ext_mask, s->player & ext_mask, s->immortal, s->wide);
  dead = s->player & opponent_unconditional;
  if (dead & s->target) {
    result = TAKE_TARGET;
  }
  if (s->wide) {
    s->opponent |= liberties_16(dead, opponent_unconditional);
  } else {
    s->opponent |= liberties(dead, opponent_unconditional);
  }
  s->player ^= dead;
  s->immortal |= s->opponent & opponent_unconditional;

  // Remove unconditionally alive stones and their eye-space from consideration
  s->logical_area ^= s->logical_area & (player_unconditional | opponent_unconditional);

  return result;
}

bool is_legal(state *s) {
  if (s->passes < 0 || s->passes > 2) {
    return false;
  }
  if (s->button != -1 && s->button != 0 && s->button != 1) {
    return false;
  }
  if ((s->logical_area | s->player | s->opponent | s->ko | s->immortal | s->external) & ~s->visual_area) {
    return false;
  }
  stones_t empty = ~(s->player | s->opponent);
  if ((s->immortal | s->external | s->target) & empty) {
    return false;
  }
  empty = (s->visual_area & empty) | s->external;

  int num_chains = 0;
  stones_t *cs;
  if (s->wide) {
    cs = chains_16(s->player & ~s->external, &num_chains);
  } else {
    cs = chains(s->player & ~s->external, &num_chains);
  }
  for (int i = 0; i < num_chains; ++i) {
    if (cs[i] & s->immortal) {
      continue;
    }
    if (!popcount(s->wide ? liberties_16(cs[i], empty) : liberties(cs[i], empty))) {
      return false;
    }
  }
  free(cs);

  bool ko_found = false;
  if (s->wide) {
    cs = chains_16(s->opponent & ~s->external, &num_chains);
  } else {
    cs = chains(s->opponent & ~s->external, &num_chains);
  }
  for (int i = 0; i < num_chains; ++i) {
    if (cs[i] & s->immortal) {
      continue;
    }
    stones_t libs = s->wide ? liberties_16(cs[i], empty) : liberties(cs[i], empty);
    if (!popcount(libs)) {
      return false;
    }

    // Bit magic to check that a single stone has ko as its liberty
    if (
      (libs == s->ko) &&
      ((cs[i] & (cs[i] - 1ULL)) == 0ULL)
     ) {
      ko_found = true;
    }
  }
  free(cs);

  // Bit magic to check that ko is a single square if present
  if (s->ko && (!ko_found || ((s->ko & (s->ko - 1ULL)) != 0ULL))) {
    return false;
  }

  return true;
}

void mirror_v(state *s) {
  if (s->wide) {
    s->visual_area = stones_mirror_v_16(s->visual_area);
    s->logical_area = stones_mirror_v_16(s->logical_area);
    s->player = stones_mirror_v_16(s->player);
    s->opponent = stones_mirror_v_16(s->opponent);
    s->ko = stones_mirror_v_16(s->ko);
    s->target = stones_mirror_v_16(s->target);
    s->immortal = stones_mirror_v_16(s->immortal);
    s->external = stones_mirror_v_16(s->external);
  } else {
    s->visual_area = stones_mirror_v(s->visual_area);
    s->logical_area = stones_mirror_v(s->logical_area);
    s->player = stones_mirror_v(s->player);
    s->opponent = stones_mirror_v(s->opponent);
    s->ko = stones_mirror_v(s->ko);
    s->target = stones_mirror_v(s->target);
    s->immortal = stones_mirror_v(s->immortal);
    s->external = stones_mirror_v(s->external);
  }
}

void mirror_h(state *s) {
  if (s->wide) {
    s->visual_area = stones_mirror_h_16(s->visual_area);
    s->logical_area = stones_mirror_h_16(s->logical_area);
    s->player = stones_mirror_h_16(s->player);
    s->opponent = stones_mirror_h_16(s->opponent);
    s->ko = stones_mirror_h_16(s->ko);
    s->target = stones_mirror_h_16(s->target);
    s->immortal = stones_mirror_h_16(s->immortal);
    s->external = stones_mirror_h_16(s->external);
  } else {
    s->visual_area = stones_mirror_h(s->visual_area);
    s->logical_area = stones_mirror_h(s->logical_area);
    s->player = stones_mirror_h(s->player);
    s->opponent = stones_mirror_h(s->opponent);
    s->ko = stones_mirror_h(s->ko);
    s->target = stones_mirror_h(s->target);
    s->immortal = stones_mirror_h(s->immortal);
    s->external = stones_mirror_h(s->external);
  }
}

void mirror_d(state *s) {
  if (s->wide) {
    fprintf(stderr, "Wide diagonal mirroring not implemented");
  }
  s->visual_area = stones_mirror_d(s->visual_area);
  s->logical_area = stones_mirror_d(s->logical_area);
  s->player = stones_mirror_d(s->player);
  s->opponent = stones_mirror_d(s->opponent);
  s->ko = stones_mirror_d(s->ko);
  s->target = stones_mirror_d(s->target);
  s->immortal = stones_mirror_d(s->immortal);
  s->external = stones_mirror_d(s->external);
}

bool can_mirror_d(const state *s) {
  return !(s->visual_area & EAST_STRIP) && !s->wide;
}

void snap(state *s) {
  if (!s->visual_area) {
    return;
  }
  int shift = 0;
  stones_t wall = s->wide ? WEST_WALL_16 : WEST_WALL;
  while (!(s->visual_area & wall)) {
    s->visual_area >>= 1;
    shift += 1;
  }
  if (s->wide) {
    while (!(s->visual_area & NORTH_WALL_16)) {
      s->visual_area >>= V_SHIFT_16;
      shift += V_SHIFT_16;
    }
  } else {
    while (!(s->visual_area & NORTH_WALL)) {
      s->visual_area >>= V_SHIFT;
      shift += V_SHIFT;
    }
  }
  s->logical_area >>= shift;
  s->player >>= shift;
  s->opponent >>= shift;
  s->ko >>= shift;
  s->target >>= shift;
  s->immortal >>= shift;
  s->external >>= shift;
}
