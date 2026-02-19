#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/compressed_graph.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"

// Read-only version of compressed_graph memory mapped directly from the filesystem

typedef struct compressed_graph_reader {
  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;  // NOTE: (m)allocated in RAM

  // List of unique value ranges in the graph
  value *value_map;  // NOTE: (m)allocated in RAM

  // Pre-computed key generator
  compressed_keyspace keyspace;  // NOTE: Only partially (m)allocated, do not free()

  // Identifiers of node values inside of self.value_map
  value_id_t *value_ids;  // NOTE: Not (m)allocated, do not free()

  // For resource acquisition and release
  struct stat sb;

  // File descriptor for resource acquisition and release
  int fd;

  // Memory map handle for resource acquisition and release
  char *buffer;
} compressed_graph_reader;

// Information about a potential move
typedef struct move_info {
  coordinates coords;
  float low_gain;
  float high_gain;
  bool low_ideal;
  bool high_ideal;
  bool forcing;
} move_info;

// Write a solved compressed_graph instance to a stream in a format expected by the reader
size_t write_compressed_graph(const compressed_graph *restrict cg, FILE *restrict stream);

// Load a compressed_graph_reader from the given file
compressed_graph_reader load_compressed_graph_reader(const char *filename);

// Get the value range of a state in a memory mapped game graph
value get_compressed_graph_reader_value(const compressed_graph_reader *cgr, const state *s);

// Get information about the potential moves in a game state. Aesthetics are compensated for
move_info* compressed_graph_reader_move_infos(const compressed_graph_reader *cgr, const state *s, int *num_move_infos);

// Release resources associated with a compressed_graph_reader instance
void unload_compressed_graph_reader(compressed_graph_reader *cgr);

// Work-around for having to re-define `struct compressed_graph_reader` in Python ctypes
compressed_graph_reader* allocate_compressed_graph_reader(const char *filename);

// Lazy developer detected
stones_t* compressed_graph_reader_python_stuff(compressed_graph_reader *cgr, state *root, int *num_moves);
