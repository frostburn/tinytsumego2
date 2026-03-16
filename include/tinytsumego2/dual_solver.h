#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/keyspace.h"

#define MAX_COMPENSATION_DEPTH (6)

#define BATCH_SIZE (256)

typedef enum {
  COMPRESSED_KEYSPACE,
  SYMMETRIC_KEYSPACE
} keyspace_type;

// An implicit game graph. All enumerable game states are evaluated even if they're not reachable from the root.
// Ko-threats are evaluated favoring either player.
// Both simple and forcing tactics are evaluated.
// The end-result should contain everything necessary for determining the status of a group or displaying information to an end-user.
typedef struct dual_graph {
  // Pre-computed key generator
  keyspace_type type;
  union {
    abstract_keyspace _;
    compressed_keyspace compressed;
    symmetric_keyspace symmetric;
  } keyspace;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // The values are indexed by compressed keys
  table_value *plain_values;
  table_value *forcing_values;

  // Polymorphic methods
  size_t (*to_key)(struct dual_graph *dg, const state *s);
  state (*from_key)(struct dual_graph *dg, size_t key);
  bool (*was_legal)(struct dual_graph *dg, size_t key);
  size_t (*remap_key)(struct dual_graph *dg, size_t key);
  state (*from_fast_key)(struct dual_graph *dg, size_t key);
  bool (*in_atari)(const state *s);
  bool (*can_take)(const state *s);

  // Parallel evaluation buffers
  size_t batch_fast_keys[BATCH_SIZE];
  size_t batch_keys[BATCH_SIZE];
  table_value batch_plain[BATCH_SIZE];
  table_value batch_forcing[BATCH_SIZE];
} dual_graph;

// Print the contents of a dual game graph
void print_dual_graph(dual_graph *dg);

// Create a dual game graph based on a root state.
dual_graph create_dual_graph(const state *root, keyspace_type type);

// Work-around for having to re-define `struct dual_graph` in Python ctypes
dual_graph* allocate_dual_graph(const state *root);

// Get the value range of a state in the game graph
value get_dual_graph_value(dual_graph *dg, const state *s, tactics ts);

// Navigate to an end state from the given starting state assuming the player cannot repeat moves
state dual_graph_low_terminal(dual_graph *dg, const state *origin, tactics ts);

// Navigate to an end state from the given starting state assuming the player can repeat moves
state dual_graph_high_terminal(dual_graph *dg, const state *origin, tactics ts);

// Apply negamax to a dual game graph. Returns `false` if the graph has converged.
bool iterate_dual_graph(dual_graph *dg, bool verbose);

value get_dual_graph_area_value(dual_graph *dg, const state *s);

// Apply negamax to a dual game graph with true area scoring after two consecutive passes. Returns `false` if the graph has converged.
bool area_iterate_dual_graph(dual_graph *dg, bool verbose);

// Free memory allocated by a dual game graph
void free_dual_graph(dual_graph *dg);
