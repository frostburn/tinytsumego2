#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/keyspace.h"

#define MAX_COMPENSATION_DEPTH (6)

// An implicit game graph. All enumerable game states are evaluated even if they're not reachable from the root.
typedef struct complete_graph {
  // Pre-computed key generator
  tight_keyspace keyspace;

  // Tactics used during solving
  tactics tactics;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // The values are indexed by tight keys
  table_value *values;
} complete_graph;

// Print the contents of a complete game graph
void print_complete_graph(complete_graph *cg);

// Create a complete game graph based on a root state.
// The second argument controls the bonus for delaying inevitable target loss.
complete_graph create_complete_graph(const state *root, tactics ts);

// Get the value range of a state in the game graph
value get_complete_graph_value(complete_graph *cg, const state *s);

// Apply negamax to a complete game graph until it converges.
// Second argument stops iteration once the root value has converged.
void solve_complete_graph(complete_graph *fg, bool root_only, bool verbose);

// Free memory allocated by a complete game graph
void free_complete_graph(complete_graph *fg);
