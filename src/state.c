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
      }
      else {
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
        printf(" ,");
      } else {
        printf("\x1b[35m");
        printf(" .");
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

move_result make_move(state *s, const stones_t move) {
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
      result = PASS;
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

  // Check if move inside empty logical area
  if (move & (s->player | s->opponent | ~s->logical_area)) {
      return ILLEGAL;
  }

  s->player |= move;
  s->ko = 0;

  // Opponent's stones killed
  stones_t kill = 0;

  // Potential liberties for opponent's stones (visual non-logical liberties count as permanent)
  stones_t empty = s->visual_area & ~s->player; 

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

  // Bit magic to check if a single stone was killed and the played stone was left in atari
  if (
    (kill & (kill - 1ULL)) == 0 &&
    liberties(move, s->logical_area & ~s->opponent) == kill
   ) {
    s->ko = kill;
  }

  // Check legality
  chain = flood(move, s->player);
  if (!liberties(chain, s->visual_area & ~s->opponent) && !(chain & s->immortal)) {
    // Oops! Revert state
    s->player = old_player;
    s->opponent = old_opponent;
    s->ko = old_ko;
    s->ko_threats = old_ko_threats;
    return ILLEGAL;
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
