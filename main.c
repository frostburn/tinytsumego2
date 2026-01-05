#include <stdio.h>
#include <stdbool.h>

/*
 * Tsumego (go problems) are arranged inside a 9x7 goban (playing area).
 * Stones (playing pieces) are represented as bit boards (unsigned integers).
 * Every bit (power of two) signifies the presence or absence of a stone.
 */
#define WIDTH (9)
#define HEIGHT (7)
#define STATE_SIZE (WIDTH * HEIGHT)

// Bit shifts associated with "physical" shifts of stones on the goban
#define H_SHIFT (1ULL)
#define V_SHIFT (WIDTH)
#define D_SHIFT (WIDTH - 1ULL)

// Bit boards associated with goban geometry
#define NORTH_WALL ((1ULL << WIDTH) - 1ULL)
#define WEST_WALL (0x40201008040201ULL)
#define WEST_BLOCK (0X3FDFEFF7FBFDFEFF)

// Horizontal strips
#define H0 (NORTH_WALL)
#define H1 (H0 << V_SHIFT)
#define H2 (H1 << V_SHIFT)
#define H3 (H2 << V_SHIFT)
#define H4 (H3 << V_SHIFT)
#define H5 (H4 << V_SHIFT)
#define H6 (H5 << V_SHIFT)

// Vertical strips
#define V0 (WEST_WALL)
#define V1 (V0 << H_SHIFT)
#define V2 (V1 << H_SHIFT)
#define V3 (V2 << H_SHIFT)
#define V4 (V3 << H_SHIFT)
#define V5 (V4 << H_SHIFT)
#define V6 (V5 << H_SHIFT)
#define V7 (V6 << H_SHIFT)
#define V8 (V7 << H_SHIFT)

// 63 bits of stones (1 bit wasted)
typedef unsigned long long int stones_t;

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

  // Number of consecutive passes made.
  int passes;

  // Number of external ko threats available. Negative numbers signify that the opponent has ko threats.
  int ko_threats;

  // Flag to indicate if the button is available. Awarded to the first player to make a passing move. Worth ½ points of area score.
  bool button;
} state;

// Print a bit board with "." for 0 bits and "@" for 1 bits. An extra row included for the non-functional 64th bit.
void print_stones(const stones_t stones) {
  // Column headers
  printf(" ");
  for (int i = 0; i < WIDTH; i++) {
      printf(" %c", 'A' + i);
  }
  printf("\n");

  for (int i = 0; i < 64; i++) {
    // Row headers. Zero indexed from top to bottom
    if (i % V_SHIFT == 0) {
      printf("%d", i / V_SHIFT);
    }

    // Stone indicators
    if ((1ULL << i) & stones) {
      printf(" @");
    }
    else {
      printf(" .");
    }

    if (i % V_SHIFT == V_SHIFT - 1){
      printf("\n");
    }
  }
  printf("\n");
}

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

int main() {
  printf("Hello, World!\n");

  print_stones(NORTH_WALL);

  state s;
  s.visual_area = 1 | 2 | 4;
  s.logical_area = 1 | 2;
  s.player = 0;
  s.opponent = 1;
  s.ko = 0;
  s.target = 0;
  s.immortal = 0;
  s.passes = 0;
  s.ko_threats = 0;
  s.button = true;

  print_state(&s, false);

  printf("thx bye\n");
}
