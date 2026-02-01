#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"

#define MIN_CAPACITY (128)
#define MAX_TAIL_SIZE (8192)

// Score range for a given game state. States with loops may not converge to a single score.
typedef struct value {
  float low;
  float high;
} value;

typedef struct full_graph {
  // Root state for key generation
  state root;

  // Use delay tactics during solving
  bool use_delay;

  // Use the struggle algorithm to prune dead targets (less memory / more compute trade-off)
  bool use_struggle;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // Nodes are collected into a queue during graph expansion
  size_t queue_length;
  size_t queue_capacity;
  state *queue;

  // Graph access is screened using a bit set
  unsigned char *seen;

  // Nodes are fully sorted once the graph has been expanded
  size_t num_nodes;
  size_t nodes_capacity;

  // States are obtained by fully expanding a root state
  state *states;

  // Values are computed after expansion to save memory
  value *values;
} full_graph;

// Print the contents of a full game graph
void print_full_graph(full_graph *fg);

// Create a full game graph based on a root state and schedule it for expansion.
// The second argument controls the bonus for delaying inevitable target loss.
// The third argument controls culling of states where the target is dead.
full_graph create_full_graph(const state *root, bool use_delay, bool use_struggle);

// Add a state to the game graph and schedule it for expansion if it's a new one
void add_full_graph_state(full_graph *fg, const state *s);

// Expand a full graph based on the state(s) enqueued
void expand_full_graph(full_graph *fg);

// Get the value range of a state in a game graph
value get_full_graph_value(full_graph *fg, const state *s);

// Apply negamax to an expanded full game graph until it converges
void solve_full_graph(full_graph *fg, bool verbose);

// Free memory allocated by a full game graph
void free_full_graph(full_graph *fg);
