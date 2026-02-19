#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/keyspace.h"

#define MAX_COMPENSATION_DEPTH (6)

// An implicit game graph. All enumerable game states are evaluated even if they're not reachable from the root.
// Ko-threats are evaluated favoring either player.
// The end-result should contain everything necessary for determining the status of a group or displaying information to an end-user.
typedef struct compressed_graph {
  // Pre-computed key generator
  compressed_keyspace keyspace;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // The values are indexed by compressed keys
  table_value *values;
} compressed_graph;

// Print the contents of a compressed game graph
void print_compressed_graph(compressed_graph *cg);

// Create a compressed game graph based on a root state.
compressed_graph create_compressed_graph(const state *root);

// Work-around for having to re-define `struct compressed_graph` in Python ctypes
compressed_graph* allocate_compressed_graph(const state *root);

// Get the value range of a state in the game graph
value get_compressed_graph_value(compressed_graph *cg, const state *s);

// Apply negamax to a compressed game graph. Returns `false` if the graph has converged.
bool iterate_compressed_graph(compressed_graph *cg, bool verbose);

// Free memory allocated by a compressed game graph
void free_compressed_graph(compressed_graph *cg);
