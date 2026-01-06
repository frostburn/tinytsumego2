#include <stdbool.h>
#include "tinytsumego2/stones.h"

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
void print_state(const state *s, bool white_to_play);

// Make a single move in a game state
// @param s: current game state
// @param move: bit board with a single bit flipped for the move to play or the zero board for a pass
// @returns: A flag indicating if the move was legal
move_result make_move(state *s, const stones_t move);
