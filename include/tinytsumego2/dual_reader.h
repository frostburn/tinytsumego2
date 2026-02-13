#pragma once
#include <stdio.h>
#include <sys/stat.h>
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

// Write a solved dual_graph instance to a stream in a format expected by the reader
size_t write_dual_graph(const dual_graph *restrict dg, FILE *restrict stream);

// Load a dual_graph_reader from the given file
dual_graph_reader load_dual_graph_reader(const char *filename);

// Get the value range of a state in a memory mapped game graph
value get_dual_graph_reader_value(const dual_graph_reader *dgr, const state *s, tactics ts);

// Release resources associated with a dual_graph_reader instance
void unload_dual_graph_reader(dual_graph_reader *dgr);
