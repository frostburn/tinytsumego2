#pragma once
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/stones.h"
#include "tinytsumego2/util.h"
#include <stdio.h>
#include <sys/stat.h>

/**
 * @file dual_reader.h
 * @brief Memory-mapped read-only access to serialized dual solver output.
 */

/** @brief Sentinel used when a value lookup must fall through to the overflow table. */
#define VALUE_ID_SENTINEL (USHRT_MAX)

/** @brief Plain and forcing value bounds for one state. */
typedef struct dual_value {
  value plain;
  value forcing;
} dual_value;

/** @brief Plain and forcing Q7 value bounds for one state. */
typedef struct dual_table_value {
  table_value plain;
  table_value forcing;
} dual_table_value;

/**
 * @brief Build-once associative structure optimized for integer keys
 *        associated with a small number of arbitrary values.
 */
typedef struct frozen_hash_table {
  /** @brief Number of entries in the dense bulk section. */
  size_t bulk_map_size;
  /** @brief Dense mapping from value identifiers to stored values. */
  dual_table_value *bulk_map;

  /** @brief Dense ids or VALUE_ID_SENTINEL when lookup must fall through. */
  value_id_t *bulk_ids;

  /** @brief Number of overflow entries in the sparse tail section. */
  size_t tail_size;
  /** @brief Values stored in the sparse tail section. */
  dual_table_value *tail_values;

  /** @brief Sorted odd-index tail keys used to derive tail lookup positions. */
  size_t *tail_keys;
} frozen_hash_table;

/**
 * @brief Read-only view of a serialized dual_graph.
 */
typedef struct dual_graph_reader {
  /** @brief Number of legal root moves. */
  int num_moves;
  /** @brief Root move list allocated in RAM. */
  stones_t *moves;

  /** @brief Keyspace representation used by the serialized graph. */
  keyspace_type type;
  union {
    abstract_keyspace _;
    compressed_keyspace compressed;
    symmetric_keyspace symmetric;
  } keyspace;

  /** @brief Compressed lookup table from keys to dual values. */
  frozen_hash_table value_table;

  /** @brief File metadata for resource management. */
  struct stat sb;

  /** @brief File descriptor backing the memory map. */
  int fd;

  /** @brief Pointer to the memory-mapped file contents. */
  char *buffer;

  /** @brief Convert a state into the serialized graph's key space. */
  size_t (*to_key)(const struct dual_graph_reader *dgr, const state *s);
} dual_graph_reader;

/** @brief Information about one candidate move returned to clients. */
typedef struct move_info {
  coordinates coords;
  float low_gain;
  float high_gain;
  bool low_ideal;
  bool high_ideal;
  bool forcing;
} move_info;

/**
 * @brief Serialize a solved dual graph and its frozen hash table to a stream.
 *
 * @param dg Solved dual graph to serialize.
 * @param fht Frozen hash table paired with `dg`.
 * @param stream Output stream receiving the serialized bytes.
 * @return Number of bytes written.
 */
size_t write_dual_graph(const dual_graph *restrict dg, const frozen_hash_table *restrict fht, FILE *restrict stream);

/**
 * @brief Populate structured fields from the raw memory-mapped buffer.
 *
 * @param dgr Reader whose `buffer` points at serialized dual-graph data.
 */
void unbuffer_dual_graph_reader(dual_graph_reader *dgr);

/**
 * @brief Load a dual graph reader from a serialized file.
 *
 * @param filename Path to a serialized dual-graph file.
 * @return Reader backed by a memory-mapped file.
 */
dual_graph_reader load_dual_graph_reader(const char *filename);

/**
 * @brief Look up the plain and forcing value bounds for a state.
 *
 * @param dgr Loaded dual-graph reader.
 * @param s State to evaluate.
 * @return Plain and forcing value bounds for `s`.
 */
dual_value get_dual_graph_reader_value(const dual_graph_reader *dgr, const state *s);

/**
 * @brief Compute annotated move information for a position.
 *
 * The returned array is heap allocated and must be freed by the caller.
 *
 * @param dgr Loaded dual-graph reader.
 * @param s State whose candidate moves should be annotated.
 * @param num_move_infos Output parameter receiving the number of entries.
 * @return Newly allocated move-info array.
 */
move_info *dual_graph_reader_move_infos(const dual_graph_reader *dgr, const state *s, int *num_move_infos);

/**
 * @brief Release resources associated with a dual graph reader.
 *
 * @param dgr Reader to unload.
 */
void unload_dual_graph_reader(dual_graph_reader *dgr);

/**
 * @brief Allocate a dual graph reader for Python ctypes bindings.
 *
 * @param filename Path to a serialized dual-graph file.
 * @return Heap-allocated reader pointer.
 */
dual_graph_reader *allocate_dual_graph_reader(const char *filename);

/**
 * @brief Return raw move data for legacy Python integration helpers.
 *
 * @param dgr Loaded dual-graph reader.
 * @param root Output parameter receiving the reader root state.
 * @param num_moves Output parameter receiving the move count.
 * @return Pointer to the reader's root move list.
 */
stones_t *dual_graph_reader_python_stuff(dual_graph_reader *dgr, state *root, int *num_moves);

/**
 * @brief Follow optimal play to a terminal state when repetitions are disallowed.
 *
 * @param dgr Loaded dual-graph reader.
 * @param origin Starting state.
 * @param ts Tactical scoring mode.
 * @return Terminal state reached under optimal play.
 */
state dual_graph_reader_low_terminal(dual_graph_reader *dgr, const state *origin, tactics ts);

/**
 * @brief Follow optimal play to a terminal state when repetitions are allowed.
 *
 * @param dgr Loaded dual-graph reader.
 * @param origin Starting state.
 * @param ts Tactical scoring mode.
 * @return Terminal state reached under optimal play.
 */
state dual_graph_reader_high_terminal(dual_graph_reader *dgr, const state *origin, tactics ts);

/**
 * @brief Compare two dual_table_value instances. Compatible with `qsort()`.
 *
 * @param a_ Pointer to the first value.
 * @param b_ Pointer to the second value.
 * @return Negative, zero, or positive ordering result.
 */
int compare_dual_table_values(const void *a_, const void *b_);

/**
 * @brief Build a frozen hash table for `write_dual_graph()` without bulk_ids.
 *
 * @param dg Solved dual graph to compress.
 * @param num_unique Output parameter receiving the number of unique values.
 * @return Frozen hash table with the bulk map and tail sections initialized.
 */
frozen_hash_table prepare_frozen_hash(const dual_graph *dg, size_t *num_unique);

/**
 * @brief Look up a dual-table value by original graph key.
 *
 * @param fht Frozen hash table to query.
 * @param key Original graph key.
 * @return Stored dual-table value for `key`.
 */
dual_table_value get_frozen_hash_value(const frozen_hash_table *fht, size_t key);

/**
 * @brief Normalize client state for internal consumption.
 *
 * @param dgr Loaded dual-graph reader.
 * @param s Game state provided by the client.
 * @return State stripped of superfluous features that still results in the same delta signature as far as optimal gameplay is concerned.
 */
state strip_aesthetics(const dual_graph_reader *dgr, const state *s);
