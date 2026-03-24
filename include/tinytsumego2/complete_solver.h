#pragma once
#include "tinytsumego2/keyspace.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/state.h"
#include <stdbool.h>

/**
 * @file complete_solver.h
 * @brief Full game-graph solver for single-value analysis.
 */

/** @brief Maximum search depth for keyspace-sparseness compensation. */
#define MAX_COMPENSATION_DEPTH (6)

/**
 * @brief Implicit game graph where every enumerable state is evaluated.
 */
typedef struct complete_graph {
  /** @brief Precomputed key generator for this root state. */
  tight_keyspace keyspace;

  /** @brief Tactical scoring mode used while solving. */
  tactics tactics;

  /** @brief Number of legal root moves represented in `moves`. */
  int num_moves;
  /** @brief Legal root moves. Always includes `pass()`. */
  stones_t *moves;

  /** @brief Node values indexed by tight-key index. */
  table_value *values;
} complete_graph;

/** @brief Print the contents of a complete game graph. */
void print_complete_graph(complete_graph *cg);

/**
 * @brief Build a complete game graph rooted at `root`.
 *
 * @param root Root state for enumeration.
 * @param ts Tactical scoring mode.
 * @return Newly created complete graph. Use `free_complete_graph()` when done.
 */
complete_graph create_complete_graph(const state *root, tactics ts);

/** @brief Look up the solved value range for a state in a complete graph. */
value get_complete_graph_value(complete_graph *cg, const state *s);

/**
 * @brief Iterate negamax until convergence.
 *
 * @param fg Graph to solve.
 * @param root_only Stop once the root value has converged.
 * @param verbose Print progress information while iterating.
 */
void solve_complete_graph(complete_graph *fg, bool root_only, bool verbose);

/** @brief Release all heap allocations owned by a complete graph. */
void free_complete_graph(complete_graph *fg);
