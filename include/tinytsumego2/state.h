#pragma once

#include <stdbool.h>
#include <stdlib.h>
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

  // External liberties. Should be adjacent to the target. Always counts as empty space.
  // Player/opponent flags indicate who can fill in the liberties. Fill order is normalized.
  stones_t external;

  // Number of consecutive passes made. Clearing a ko or taking the button doesn't qualify.
  int passes;

  // Number of external ko threats available. Negative numbers signify that the opponent has ko threats.
  int ko_threats;

  // Indicate the owner of the button. Awarded to the first player to make a passing move. Worth ½ points of area score.
  // -1: opponent has button
  //  0: button not awarded yet
  // +1: player has button
  int button;

  // Indicate which color "player" refers to.
  bool white_to_play;
} state;

// Result of making a move
typedef enum move_result
{
  ILLEGAL,
  CLEAR_KO,
  TAKE_BUTTON,
  PASS,
  SECOND_PASS,
  FILL_EXTERNAL,
  NORMAL,
  KO_THREAT_AND_RETAKE,
  TAKE_TARGET,
  TARGET_LOST
} move_result;

// Print a game state with ANSI colors
void print_state(const state *s);

// Print the representation string of a game state
void repr_state(const state *s);

// Parse a game state from a string
// . Empty playable space
// , Empty unplayable space
// * Empty space temporarily unavailable due to a ko
// x Void used to pad lines to full 9 squares
// @ Black stone
// b Target black stone
// B Immortal black stone
// 0 White stone
// w Target white stone
// W Immortal white stone
state parse_state(const char *visuals);

// Make a single move in a game state
// @param s: current game state
// @param move: bit board with a single bit flipped for the move to play or the zero board for a pass
// @returns: A flag indicating if the move was legal
move_result make_move(state *s, const stones_t move);

// Return the unique index identifying a child state of a root state
// Only a single pass is allowed. Do not index leaf nodes of the tree!
size_t to_key(state *root, state *child);

// Return the size of the key space of the given root state
size_t keyspace_size(state *root);

// Count the difference between the number of player's stones and liberties and opponent's stones and liberties
int chinese_liberty_score(const state *s);

// Return true if two game states are the same. False otherwise.
bool equals(const state *a, const state *b);

// Compare two child states of a common root state. Compatible with qsort.
int compare(const void *a_, const void *b_);


// Get a hash of a state. Collisions can happen but child states of a common root state should hash reasonably well.
stones_t hash_a(const state *s);

// Get a hash of a state. Collisions can happen but child states of a common root state should hash reasonably well.
stones_t hash_b(const state *s);

// Apply Benson's Algorithm for Unconditional Life by removing unconditional eye-space from logical playing area.
// Returns TAKE_TARGET or TARGET_LOST if the process captures target stones as a side effect.
move_result apply_benson(state *s);
