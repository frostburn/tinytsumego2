#pragma once

#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/state.h"
#include <stdbool.h>

/**
 * @file collection.h
 * @brief Hard-coded tsumego collections exposed through the HTTP bridge.
 *
 * Collections are normalized to the upper-left corner of a 19x19 board with
 * the rest of the board left empty.
 */

/**
 * @brief One named tsumego entry within a collection.
 */
typedef struct tsumego {
  /** @brief URL-friendly identifier. */
  char *slug;
  /** @brief User-facing subtitle. */
  char *subtitle;
  /** @brief Problem state enumerable from the collection root. */
  state state;
  /** @brief True when the book/computer should move first. */
  bool bot_to_play;
  /** @brief Expected solved value used when validating generated data. */
  value value;
} tsumego;

/**
 * @brief A named set of related tsumego problems.
 */
typedef struct collection {
  /** @brief URL-friendly identifier. */
  char *slug;
  /** @brief User-facing title. */
  char *title;
  /** @brief Root position in the upper-left corner. */
  state root;
  /** @brief True when the shape can be embedded in a full 19x19 board. */
  bool can_stretch;
  /** @brief Number of sub-problems reachable from the root. */
  size_t num_tsumegos;
  /** @brief Array of tsumego entries. */
  tsumego *tsumegos;
  /** @brief Keyspace type to use when solving the collection. */
  keyspace_type type;
} collection;

/**
 * @brief Return the built-in collections used by the API bridge.
 *
 * @param num_collections Output parameter receiving the collection count.
 * @return Pointer to a static array of collections.
 */
collection *get_collections(size_t *num_collections);
