#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/stones16.h"

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

  // The zero bitboard. Or a single bit signifying the illegal ko recapture.
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

  // Indicate the owner of the button. Awarded to the first player to make a passing move. Worth a quarter-point of area score.
  // -1: opponent has button
  //  0: button not awarded yet
  // +1: player has button
  int button;

  // Indicate which color "player" refers to.
  bool white_to_play;

  // Use stones with a width of 16 instead
  bool wide;
} state;

// Result of making a move (ordered to facilitate game graph expansion)
typedef enum move_result
{
  // Non-moves
  ILLEGAL,

  // Game-ending moves
  TARGET_LOST, // Not a legal move but can happen as an optimization
  SECOND_PASS,
  TAKE_TARGET,

  // Non-constructive moves
  CLEAR_KO,
  TAKE_BUTTON,
  PASS,

  // Constructive moves
  FILL_EXTERNAL,
  NORMAL,
  KO_THREAT_AND_RETAKE,
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
// Assumes external liberties (if any) are arranged in a contiguous block
// Only a single pass is allowed. Do not index leaf nodes of the tree!
size_t to_key(const state *root, const state *child);

// Return the size of the key space of the given root state
size_t keyspace_size(const state *root);

// Return a unique index of a simple child state of a root state.
// No passes or ko allowed.
// Button must be owned by the player if taken.
size_t to_tight_key(const state *root, const state *child, const bool symmetric_threats);

// Reconstruct a simple state based on its unique index w.r.t. a root state.
state from_tight_key(const state *root, size_t key, const bool symmetric_threats);

// Return the size of the simple key space of the given root state
size_t tight_keyspace_size(const state *root, const bool symmetric_threats);

// Compare two unique indices. Compatible with qsort.
int compare_keys(const void *a_, const void *b_);

// Count the difference between the number of player's stones and liberties and opponent's stones and liberties
int chinese_liberty_score(const state *s);

// Chinese-like score but ownership is anticipated
int compensated_liberty_score(const state *s);

// Return true if two game states are the same. False otherwise.
bool equals(const state *a, const state *b);

// Compare two child states of a common root state. Compatible with qsort.
int compare(const void *a_, const void *b_);

// Compare two simple child states of a common root state. Compatible with qsort.
int compare_simple(const void *a_, const void *b_);

// Get a hash of a state. Collisions can happen but child states of a common root state should hash reasonably well.
stones_t hash_a(const state *s);

// Get a hash of a state. Collisions can happen but child states of a common root state should hash reasonably well.
stones_t hash_b(const state *s);

// Apply Benson's Algorithm for Unconditional Life by removing unconditional eye-space from logical playing area.
// Returns TAKE_TARGET or TARGET_LOST if the process captures target stones as a side effect.
move_result apply_benson(state *s);

// Play out regions surrounded by immortal stones of the opponent.
move_result normalize_immortal_regions(state *root, state *s);

// Returns `true` if the state is legal. No chains without liberties etc.
bool is_legal(state *s);

// Mirror the state vertically in-place
void mirror_v(state *s);

// Mirror the state horizontally in-place
void mirror_h(state *s);

// Mirror the state diagonally in-place
void mirror_d(state *s);

// Returns `true` if applying `mirror_d` preserves all information. Assumes the state is legal
bool can_mirror_d(const state *s);

// Snap state to the upper left corner. Assumes the state is legal
void snap(state *s);

// Compute the status of the target stones assuming the other player keeps passing
move_result struggle(const state *s);
