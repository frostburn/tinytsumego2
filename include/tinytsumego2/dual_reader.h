#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"

// Read-only version of dual_solver memory mapped directly from the filesystem

typedef struct dual_graph_reader {
  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;  // NOTE: (m)allocated in RAM

  // List of unique value ranges in the graph
  value *value_map;  // NOTE: (m)allocated in RAM

  // Pre-computed key generator
  compressed_keyspace keyspace;  // NOTE: Only partially (m)allocated, do not free()

  // Identifiers of node values inside of self.value_map
  value_id_t *plain_value_ids;  // NOTE: Not (m)allocated, do not free()
  value_id_t *forcing_value_ids;  // NOTE: Not (m)allocated, do not free()

  // For resource acquisition and release
  struct stat sb;

  // File descriptor for resource acquisition and release
  int fd;

  // Memory map handle for resource acquisition and release
  char *buffer;
} dual_graph_reader;

typedef struct dual_value {
  value plain;
  value forcing;
} dual_value;

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
size_t write_dual_graph(const dual_graph *restrict dg, FILE *restrict stream);

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
