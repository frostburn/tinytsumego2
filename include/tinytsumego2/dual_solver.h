#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/keyspace.h"

#define MAX_COMPENSATION_DEPTH (6)

// An implicit game graph. All enumerable game states are evaluated even if they're not reachable from the root.
// Ko-threats are evaluated favoring either player.
// Both simple and forcing tactics are evaluated.
// The end-result should contain everything necessary for determining the status of a group or displaying information to an end-user.
typedef struct dual_graph {
  // Pre-computed key generator
  compressed_keyspace keyspace;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // The values are indexed by compressed keys
  table_value *plain_values;
  table_value *forcing_values;
} dual_graph;

// Print the contents of a dual game graph
void print_dual_graph(dual_graph *dg);

// Create a dual game graph based on a root state.
dual_graph create_dual_graph(const state *root);

// Work-around for having to re-define `struct dual_graph` in Python ctypes
dual_graph* allocate_dual_graph(const state *root);

// Get the value range of a state in the game graph
value get_dual_graph_value(dual_graph *dg, const state *s, tactics ts);

// Apply negamax to a dual game graph. Returns `false` if the graph has converged.
bool iterate_dual_graph(dual_graph *dg, bool verbose);

// Free memory allocated by a dual game graph
void free_dual_graph(dual_graph *dg);
