#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/keyspace.h"

// TODO: This is too similar to dual_graph. Just make that one polymorphic.

#define MAX_COMPENSATION_DEPTH (6)

// An implicit game graph with symmetries reduced out of the keyspace.
// All enumerable game states are evaluated even if they're not reachable from the root.
// Ko-threats are evaluated favoring either player.
// Both simple and forcing tactics are evaluated.
// The end-result should contain everything necessary for determining the status of a group or displaying information to an end-user.
typedef struct sym_graph {
  // Pre-computed key generator
  symmetric_keyspace keyspace;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // The values are indexed by compressed keys
  table_value *plain_values;
  table_value *forcing_values;
} sym_graph;

// Create a sym game graph based on a root state.
sym_graph create_sym_graph(const state *root);

// Work-around for having to re-define `struct sym_graph` in Python ctypes
sym_graph* allocate_sym_graph(const state *root);

// Get the value range of a state in the game graph
value get_sym_graph_value(sym_graph *sg, const state *s, tactics ts);

// Navigate to an end state from the given starting state assuming the player cannot repeat moves
state sym_graph_low_terminal(sym_graph *sg, const state *origin, tactics ts);

// Navigate to an end state from the given starting state assuming the player can repeat moves
state sym_graph_high_terminal(sym_graph *sg, const state *origin, tactics ts);

// Apply negamax to a sym game graph. Returns `false` if the graph has converged.
bool iterate_sym_graph(sym_graph *sg, bool verbose);

value get_sym_graph_area_value(sym_graph *sg, const state *s);

// Apply negamax to a sym game graph with true area scoring after two consecutive passes. Returns `false` if the graph has converged.
bool area_iterate_sym_graph(sym_graph *sg, bool verbose);

// Free memory allocated by a sym game graph
void free_sym_graph(sym_graph *sg);
