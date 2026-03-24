#pragma once
#include "tinytsumego2/complete_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"
#include <stdio.h>
#include <sys/stat.h>

/**
 * @file complete_reader.h
 * @brief Memory-mapped read-only access to serialized complete solver output.
 */

/**
 * @brief Read-only view of a serialized complete_graph.
 */
typedef struct complete_graph_reader {
  /** @brief Tight keyspace metadata recreated in RAM. */
  tight_keyspace keyspace;

  /** @brief Tactical mode used when solving the graph. */
  tactics tactics;

  /** @brief Number of legal root moves. */
  int num_moves;
  /** @brief Root move list allocated in RAM. */
  stones_t *moves;

  /** @brief Unique solved value ranges allocated in RAM. */
  value *value_map;

  /** @brief Serialized indices into value_map stored inside the memory-mapped file. */
  value_id_t *value_ids;

  /** @brief File metadata for resource management. */
  struct stat sb;

  /** @brief File descriptor backing the memory map. */
  int fd;

  /** @brief Pointer to the memory-mapped file contents. */
  char *buffer;
} complete_graph_reader;

/**
 * @brief Serialize a solved complete graph to a stream.
 *
 * @return Number of bytes written.
 */
size_t write_complete_graph(const complete_graph *restrict cg, FILE *restrict stream);

/** @brief Load a complete graph reader from a serialized file. */
complete_graph_reader load_complete_graph_reader(const char *filename);

/** @brief Look up the solved value range of a state in a mapped graph. */
value get_complete_graph_reader_value(const complete_graph_reader *cgr, const state *s);

/** @brief Release the resources associated with a complete graph reader. */
void unload_complete_graph_reader(complete_graph_reader *cgr);
