#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/stones16.h"

/**
 * @file state.h
 * @brief Full game-state representation and move-generation primitives.
 */

/**
 * @brief Complete mutable game state for a tinytsumego position.
 */
typedef struct state
{
  /** @brief Visible playing area; zero bits mark board edges for rendering. */
  stones_t visual_area;

  /** @brief Legal playing area; zero bits are not legal move locations. */
  stones_t logical_area;

  /** @brief Stones belonging to the side to move. */
  stones_t player;

  /** @brief Stones belonging to the side that moved previously. */
  stones_t opponent;

  /** @brief Ko restriction, or zero when no ko is active. */
  stones_t ko;

  /** @brief Target stones to save or capture depending on the problem. */
  stones_t target;

  /** @brief Stones that remain on the board even without liberties. */
  stones_t immortal;

  /**
   * @brief External liberties treated as initially empty, color-exclusive space.
   *
   * These are mainly used to model first-line crawlspace and liberties outside
   * of the main eyespace adjacent to a target group.
   */
  stones_t external;

  /** @brief Number of consecutive passes. */
  int passes;

  /** @brief Signed count of external ko threats; negative means the opponent leads. */
  int ko_threats;

  /**
   * @brief Owner of the button bonus.
   *
   * `-1` means the opponent owns it, `0` means it is unclaimed, and `+1`
   * means the current player owns it.
   */
  int button;

  /** @brief True when `player` should be interpreted as white stones. */
  bool white_to_play;

  /** @brief Select the alternate 16x4 stone helpers when true. */
  bool wide;
} state;

/**
 * @brief Classification of the result of attempting a move.
 *
 * The values are ordered to simplify game-graph expansion.
 */
typedef enum move_result
{
  /** @brief Move was illegal. */
  ILLEGAL,

  /** @brief Optimization sentinel indicating a target has already been lost. */
  TARGET_LOST,
  /** @brief Second consecutive pass, ending the local game. */
  SECOND_PASS,
  /** @brief Move captures a target. */
  TAKE_TARGET,

  /** @brief Non-constructive move that only clears ko state. */
  CLEAR_KO,
  /** @brief Passing move that claims the button bonus. */
  TAKE_BUTTON,
  /** @brief Regular pass. */
  PASS,

  /** @brief Move fills an external liberty. */
  FILL_EXTERNAL,
  /** @brief Ordinary constructive move. */
  NORMAL,
  /** @brief Move spends a ko threat and immediately retakes. */
  KO_THREAT_AND_RETAKE,
} move_result;

/** @brief Print a game state using ANSI colors. */
void print_state(const state *s);

/** @brief Print the compact string representation of a game state. */
void repr_state(const state *s);

/**
 * @brief Parse a state from the engine's ASCII board representation.
 *
 * Accepted characters:
 * - `.` empty playable space
 * - `,` empty unplayable space
 * - `*` temporary ko point
 * - `x` void used to pad lines to full width
 * - `@` black stone
 * - `b` target black stone
 * - `B` immortal black stone
 * - `0` white stone
 * - `w` target white stone
 * - `W` immortal white stone
 *
 * @param visuals Null-terminated ASCII board description.
 * @return Parsed game state.
 */
state parse_state(const char *visuals);

/**
 * @brief Play a move in-place.
 *
 * @param s Current game state to mutate.
 * @param move Single-bit move location, or `pass()` to pass.
 * @return Result describing legality and tactical consequences of the move.
 */
move_result make_move(state *s, const stones_t move);

/**
 * @brief Encode a simple child state as a unique integer key.
 *
 * The child must be derived from `root` without passes or active ko. If the
 * button has been taken, it must belong to the player.
 */
size_t to_tight_key(const state *root, const state *child, const bool symmetric_threats);

/** @brief Decode a simple child-state key produced by `to_tight_key()`. */
state from_tight_key(const state *root, size_t key, const bool symmetric_threats);

/** @brief Return the size of the simple key space rooted at `root`. */
size_t tight_keyspace_size(const state *root, const bool symmetric_threats);

/** @brief Compare two integer keys. Compatible with `qsort()`. */
int compare_keys(const void *a_, const void *b_);

/** @brief Score stones and liberties using Chinese-style counting. */
int chinese_liberty_score(const state *s);

/** @brief Score the position while anticipating ownership of unsettled points. */
int compensated_liberty_score(const state *s);

/** @brief Count simple area without removing dead stones. */
int simple_area_score(const state *s);

/** @brief Return true when two game states are identical. */
bool equals(const state *a, const state *b);

/** @brief Compare two child states of a common root. Compatible with `qsort()`. */
int compare(const void *a_, const void *b_);

/** @brief Compare two simple child states of a common root. Compatible with `qsort()`. */
int compare_simple(const void *a_, const void *b_);

/** @brief Compute the first hash used for state bucketing and bloom filters. */
stones_t hash_a(const state *s);

/** @brief Compute the second hash used for state bucketing and bloom filters. */
stones_t hash_b(const state *s);

/**
 * @brief Apply Benson's algorithm for unconditional life.
 *
 * Removes unconditional eye-space from the logical area. The operation may also
 * determine that a target is captured as a side effect.
 */
move_result apply_benson(state *s);

/** @brief Play out regions surrounded by the opponent's immortal stones. */
move_result normalize_immortal_regions(state *root, state *s);

/** @brief Return true when the state satisfies legality constraints, including no chain without liberties. */
bool is_legal(const state *s);

/** @brief Return true when a target chain can be captured in one move. */
bool target_capturable(const state *s);

/** @brief Return true when the current player's target stones are in atari. */
bool target_in_atari(const state *s);

/** @brief Mirror a state across the horizontal axis in place. */
void mirror_v(state *s);

/** @brief Mirror a state across the vertical axis in place. */
void mirror_h(state *s);

/** @brief Mirror a state across the main diagonal in place. */
void mirror_d(state *s);

/**
 * @brief Test whether diagonal mirroring preserves all state information.
 *
 * Assumes the state is legal.
 */
bool can_mirror_d(const state *s);

/** @brief Snap a legal state to the upper-left corner in place. */
void snap(state *s);

/** @brief Resolve a target status assuming the opponent keeps passing. */
move_result struggle(const state *s);

/**
 * @brief Enumerate potential moves from a root state.
 *
 * The returned list includes `pass()`. The caller must free the returned array.
 */
stones_t* moves_of(const state *root, int *num_moves);

/** @brief Swap player/opponent roles without incrementing the pass count. */
void swap_players(state *s);
