#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"

#define BLOOM_SIZE (2097152)
#define BLOOM_MASK (2097151)
#define BLOOM_SHIFT (24)

#define MIN_CAPACITY (128)
#define MAX_TAIL_SIZE (8192)

// Score range for a given game state. States with loops may not converge to a single score.
typedef struct value {
  float low;
  float high;
} value;

typedef struct full_graph {
  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // Nodes are collected into a queue during graph expansion
  size_t queue_length;
  size_t queue_capacity;
  state *queue;

  // Graph access is screened using a bloom filter
  unsigned char *bloom;

  // Nodes are partially sorted
  size_t num_nodes;
  size_t nodes_capacity;
  size_t num_sorted;

  // States are obtained by fully expanding a root state
  state *states;

  // Values are computed after expansion to save memory
  value *values;
} full_graph;

// Add an entry to the bloom filter
void bloom_insert(unsigned char *bloom, stones_t a, stones_t b);

// Test bloom filter membership `true` indicates likely membership in the set. `false` indicates that the element definitely isn't in the set.
bool bloom_test(unsigned char *bloom, stones_t a, stones_t b);

// Create a full game graph by expanding a root state
full_graph create_full_graph(state root);

// Get the value range of a state in a game graph
value get_full_graph_value(full_graph *fg, state *s);

// Apply negamax to an expanded full game graph until it converges
void solve_full_graph(full_graph *fg);

// Free memory allocated by a full game graph
void free_full_graph(full_graph *fg);
