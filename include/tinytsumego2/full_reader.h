#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include "tinytsumego2/full_solver.h"
#include "tinytsumego2/scoring.h"

// Read-only version of full_solver memory mapped directly from the filesystem

// Use key compression and 16-bit values to save space
typedef struct light_node {
  size_t key;
  score_q7_t low;
  score_q7_t high;
} light_node;

typedef struct full_graph_reader {
  // Root state for key generation
  state root;

  // Valid moves according to the root state
  int num_moves;
  stones_t *moves;  // NOTE: (m)allocated in RAM

  // Nodes are fully sorted
  size_t num_nodes;

  // Only access keys are stored to save space
  light_node *nodes;  // NOTE: Not (m)allocated, do not free()

  // For resource acquisition and release
  struct stat sb;

  // File descriptor for resource acquisition and release
  int fd;

  // Memory map handle for resource acquisition and release
  char *buffer;
} full_graph_reader;

// Write a solved full_graph instance to a stream in a format expected by the reader
size_t write_full_graph(const full_graph *restrict fg, FILE *restrict stream);

// Load a full_graph_reader from the given file
full_graph_reader load_full_graph_reader(const char *filename);

// Get the value range of a state in a memory mapped game graph
value get_full_graph_reader_value(const full_graph_reader *fgr, const state *s);

// Release resources associated with a full_graph_reader instance
void unload_full_graph_reader(full_graph_reader *fgr);
