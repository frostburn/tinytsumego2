#include <stdio.h>
#include <stdbool.h>

// Game state
typedef struct state
{
  // The visual playing area. Zeroed bits signify the edge of the board.
  stones_t visual_area;

  // The logical playing area. Moves cannot be made inside zeroed bits. Outside stones are for decoration.
  stones_t logical_area;

  // Stones of the player to make the next move.
  stones_t player;

  // Stones of the player the made the last move.
  stones_t opponent;

  // The zero bit board. Or a single bit signifying the illegal ko recapture.
  stones_t ko;

  // Tsumego target(s) to be captured or saved depending on the problem.
  stones_t target;

  // Stones that cannot be captured even if they run out of liberties.
  stones_t immortal;

  // Number of consecutive passes made. Clearing a ko or taking the button doesn't qualify.
  int passes;

  // Number of external ko threats available. Negative numbers signify that the opponent has ko threats.
  int ko_threats;

  // Indicate the owner of the button. Awarded to the first player to make a passing move. Worth ½ points of area score.
  // -1: opponent has button
  //  0: button not awarded yet
  // +1: player has button
  int button;
} state;

// Result of making a move
typedef enum move_result
{
  ILLEGAL,
  CLEAR_KO,
  TAKE_BUTTON,
  PASS,
  NORMAL,
  KO_THREAT_AND_RETAKE,
  TAKE_TARGET
} move_result;

// Print a game state with ANSI colors
void print_state(const state *s, bool white_to_play) {
  stones_t black, white;
  if (white_to_play) {
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
      printf("\x1b[35m");
      printf(" .");
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
  if (white_to_play) {
    printf("White to play\n");
  }
  else {
    printf("Black to play\n");
  }
}

// Make a single move in a game state
// @param s: current game state
// @param move: bit board with a single bit flipped for the move to play or the zero board for a pass
// @returns: A flag indicating if the move was legal
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

  if (kill & s->target) {
    return TAKE_TARGET;
  }

  return result;
}

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
}
