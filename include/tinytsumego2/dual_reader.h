#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"

// Read-only version of dual_solver memory mapped directly from the filesystem

typedef struct dual_value {
  value plain;
  value forcing;
} dual_value;

typedef struct dual_table_value {
  table_value plain;
  table_value forcing;
} dual_table_value;

typedef struct dual_graph_reader {
  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;  // NOTE: (m)allocated in RAM

  // List of unique value ranges in the graph
  size_t value_map_size;
  dual_value *value_map;  // NOTE: (m)allocated in RAM

  // Pre-computed key generator
  keyspace_type type;
  union {
    abstract_keyspace _;
    compressed_keyspace compressed;
    symmetric_keyspace symmetric;
  } keyspace;  // NOTE: Only partially (m)allocated, do not free()

  // Identifiers of node values inside of self.value_map
  dual_value_id_t *value_ids;  // NOTE: Not (m)allocated, do not free()

  // For resource acquisition and release
  struct stat sb;

  // File descriptor for resource acquisition and release
  int fd;

  // Memory map handle for resource acquisition and release
  char *buffer;

  // Polymorphic method(s)
  size_t (*to_key)(const struct dual_graph_reader *dgr, const state *s);
} dual_graph_reader;

// Information about a potential move
typedef struct move_info {
  coordinates coords;
  float low_gain;
  float high_gain;
  bool low_ideal;
  bool high_ideal;
  bool forcing;
} move_info;

// Write a solved dual_graph instance to a stream in a format expected by the reader
size_t write_dual_graph(const dual_graph *restrict dg, const dual_table_value *restrict value_map, size_t value_map_size, FILE *restrict stream);

// Unroll `dgr->buffer` into struct fields
void unbuffer_dual_graph_reader(dual_graph_reader *dgr);

// Load a dual_graph_reader from the given file
dual_graph_reader load_dual_graph_reader(const char *filename);

// Get the value ranges of a state in a memory mapped game graph
dual_value get_dual_graph_reader_value(const dual_graph_reader *dgr, const state *s);

// Get information about the potential moves in a game state. Aesthetics are compensated for
move_info* dual_graph_reader_move_infos(const dual_graph_reader *dgr, const state *s, int *num_move_infos);

// Release resources associated with a dual_graph_reader instance
void unload_dual_graph_reader(dual_graph_reader *dgr);

// Work-around for having to re-define `struct dual_graph_reader` in Python ctypes
dual_graph_reader* allocate_dual_graph_reader(const char *filename);

// Lazy developer detected
stones_t* dual_graph_reader_python_stuff(dual_graph_reader *dgr, state *root, int *num_moves);

// Navigate to an end state from the given starting state assuming the player cannot repeat moves
state dual_graph_reader_low_terminal(dual_graph_reader *dgr, const state *origin, tactics ts);

// Navigate to an end state from the given starting state assuming the player can repeat moves
state dual_graph_reader_high_terminal(dual_graph_reader *dgr, const state *origin, tactics ts);

// Compare two dual_table_values. Compatible with qsort
int compare_dual_table_values(const void *a_, const void *b_);

// Prepare value map for `write_dual_graph()`
dual_table_value* create_value_map(dual_graph *dg, size_t *value_map_size);
