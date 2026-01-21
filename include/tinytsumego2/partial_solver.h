#pragma once
#include <stdbool.h>
#include "tinytsumego2/state.h"

#define MIN_CAPACITY (128)
#define MAX_TAIL_SIZE (8192)

// Minimum number of re-visits before children are permanently expanded
#define MIN_VISITS (4)

// Child state of a parent state with heuristic scores attached
struct child {
  state state;
  move_result move_result;
  int heuristic_penalty;
};

// Node in the game graph
typedef struct node {
  state state;
  int depth;
  float low;
  float high;
  bool low_fixed;
  bool high_fixed;
  int generation;
  int visits;
  int num_children;
  struct child *children;
} node;

// Copy of a node for ease of re-access
typedef struct node_proxy {
  state state;
  int depth;
  float low;
  float high;
  bool low_fixed;
  bool high_fixed;
  int generation;
  int visits;
  int num_children;
  struct child *children;
  size_t index;
  size_t tag;
} node_proxy;

typedef struct game_graph {
  // Root state
  state root;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;

  // Graph access is screened using a bloom filter
  unsigned char *bloom;

  // Nodes are partially sorted
  size_t num_nodes;
  size_t nodes_capacity;
  size_t num_sorted;
  node *nodes;

  // Generations control re-entry into graph loops during search
  int generation;

  // Global flag to track any updates
  bool updated;

  // This flag controls fixing value ranges once the graph fails to update
  bool fix_loops;
} game_graph;

// Sort children by heuristic penalty using qsort
int compare_children(const void *a_, const void *b_);

// Create a game graph and place the root state at depth zero
game_graph create_game_graph(const state *root);

// Print the contents of a game graph
void print_game_graph(game_graph *gg);

// Get a proxy for modifying nodes in a game graph. Allocates new nodes as needed
node_proxy get_game_graph_node(game_graph *gg, const state *s);

// Commit modifications made to the proxy into the actual graph
void update_game_graph_node(game_graph *gg, node_proxy *np);

// Improve a lower or upper bound of a node in the game graph by partially expanding the graph as needed
void improve_bound(game_graph *gg, node_proxy *np, bool lower);

// Expand the graph from the root node until its value range converges
void solve_game_graph(game_graph *gg, bool verbose);

// Free memory allocated by the game graph
void free_game_graph(game_graph *gg);
