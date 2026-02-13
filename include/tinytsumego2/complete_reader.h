#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include "tinytsumego2/complete_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"

// Read-only version of complete_solver memory mapped directly from the filesystem

typedef struct complete_graph_reader {
  // Pre-computed key generator
  tight_keyspace keyspace;  // NOTE: (m)allocated in RAM

  // Tactics used during solving
  tactics tactics;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;  // NOTE: (m)allocated in RAM

  // List of unique value ranges in the graph
  value *value_map;  // NOTE: (m)allocated in RAM

  // Identifiers of node values inside of self.value_map
  value_id_t *value_ids;  // NOTE: Not (m)allocated, do not free()

  // For resource acquisition and release
  struct stat sb;

  // File descriptor for resource acquisition and release
  int fd;

  // Memory map handle for resource acquisition and release
  char *buffer;
} complete_graph_reader;

// Write a solved complete_graph instance to a stream in a format expected by the reader
size_t write_complete_graph(const complete_graph *restrict cg, FILE *restrict stream);

// Load a complete_graph_reader from the given file
complete_graph_reader load_complete_graph_reader(const char *filename);

// Get the value range of a state in a memory mapped game graph
value get_complete_graph_reader_value(const complete_graph_reader *cgr, const state *s);

// Release resources associated with a complete_graph_reader instance
void unload_complete_graph_reader(complete_graph_reader *cgr);
