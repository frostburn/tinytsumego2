#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/keyspace.h"

/**
 * @file dual_solver.h
 * @brief Dual-value game-graph solver that tracks plain and forcing tactics.
 */

/** @brief Maximum search depth for delayed-capture compensation. */
#define MAX_COMPENSATION_DEPTH (6)
/** @brief Number of nodes processed per parallel evaluation batch. */
#define BATCH_SIZE (256)

/**
 * @brief Choice of keyspace implementation backing a dual graph.
 */
typedef enum {
  COMPRESSED_KEYSPACE,
  SYMMETRIC_KEYSPACE,
  MOCK_KEYSPACE
} keyspace_type;

/**
 * @brief Implicit game graph storing both plain and forcing values.
 *
 * All enumerable states are evaluated even when they are not reachable from the
 * root. The resulting graph contains enough information to classify status and
 * suggest end-user-visible continuations.
 */
typedef struct dual_graph {
  /** @brief Keyspace representation used for this graph. */
  keyspace_type type;
  union {
    abstract_keyspace _;
    compressed_keyspace compressed;
    symmetric_keyspace symmetric;
  } keyspace;

  /** @brief Number of legal root moves represented in `moves`. */
  int num_moves;
  /** @brief Legal root moves, typically including pass(). */
  stones_t *moves;

  /** @brief Plain tactical values indexed by compressed key. */
  table_value *plain_values;
  /** @brief Forcing tactical values indexed by compressed key. */
  table_value *forcing_values;

  /** @brief Convert a state to the key type used by this graph. */
  size_t (*to_key)(struct dual_graph *dg, const state *s);
  /** @brief Recover a state from a compressed key. */
  state (*from_key)(struct dual_graph *dg, size_t key);
  /** @brief Test whether a fast key corresponds to a legal state. */
  bool (*was_legal)(struct dual_graph *dg, size_t key);
  /** @brief Remap a fast key into the stored key space. */
  size_t (*remap_key)(struct dual_graph *dg, size_t key);
  /** @brief Recover a state from a fast key when supported. */
  state (*from_fast_key)(struct dual_graph *dg, size_t key);
  /** @brief Predicate reporting whether the side to move is in atari. */
  bool (*in_atari)(const state *s);
  /** @brief Predicate reporting whether the side to move can capture immediately. */
  bool (*can_take)(const state *s);

  /** @brief Scratch storage for batched fast keys. */
  size_t batch_fast_keys[BATCH_SIZE];
  /** @brief Scratch storage for batched remapped keys. */
  size_t batch_keys[BATCH_SIZE];
  /** @brief Scratch storage for batched plain values. */
  table_value batch_plain[BATCH_SIZE];
  /** @brief Scratch storage for batched forcing values. */
  table_value batch_forcing[BATCH_SIZE];
} dual_graph;

/** @brief Print the contents of a dual game graph. */
void print_dual_graph(dual_graph *dg);

/** @brief Create a dual game graph rooted at `root`. */
dual_graph create_dual_graph(const state *root, keyspace_type type);

/** @brief Allocate a dual graph for use from Python ctypes bindings. */
dual_graph* allocate_dual_graph(const state *root, keyspace_type type);

/** @brief Look up the solved value range of a state using the selected tactics. */
value get_dual_graph_value(dual_graph *dg, const state *s, tactics ts);

/** @brief Follow optimal play to a terminal state when repetitions are disallowed. */
state dual_graph_low_terminal(dual_graph *dg, const state *origin, tactics ts);

/** @brief Follow optimal play to a terminal state when repetitions are allowed. */
state dual_graph_high_terminal(dual_graph *dg, const state *origin, tactics ts);

/**
 * @brief Perform one negamax iteration.
 *
 * @return False when the graph has converged.
 */
bool iterate_dual_graph(dual_graph *dg, bool verbose);

/** @brief Evaluate a state using true area scoring after two consecutive passes. */
value get_dual_graph_area_value(dual_graph *dg, const state *s);

/**
 * @brief Perform one area-scoring negamax iteration.
 *
 * @return False when the graph has converged.
 */
bool area_iterate_dual_graph(dual_graph *dg, bool verbose);

/** @brief Release all heap allocations owned by a dual graph. */
void free_dual_graph(dual_graph *dg);
