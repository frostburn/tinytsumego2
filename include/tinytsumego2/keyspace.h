#pragma once

#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"
#include "tinytsumego2/symmetry.h"

/**
 * @file keyspace.h
 * @brief Helpers for mapping game states to compact integer key spaces.
 */

/** @brief Ternary conversion helper used while building tight keys. */
typedef struct tritter {
  size_t m;
  stones_t shift;
  stones_t mask;
} tritter;

/** @brief Auxiliary data structure used for dense root-relative keys. */
typedef struct tight_keyspace {
  state root;
  size_t size;
  bool symmetric_threats;
  size_t ko_m;
  int num_tritters;
  tritter *tritters;
  size_t external_m;
  size_t *external_keys;
  size_t external_prime;

  size_t prefix_m;
  state *prefixes;
  int num_blocks;
  stones_t **black_blocks;
  stones_t **white_blocks;
} tight_keyspace;

/** @brief Compression helper for monotonically increasing integer sequences. */
typedef struct monotonic_compressor {
  size_t num_checkpoints;
  size_t *checkpoints;
  size_t uncompressed_size;
  unsigned char *deltas;
  size_t size;
  double factor;
} monotonic_compressor;

/** @brief Shared prefix for compressed keyspace variants. */
typedef struct abstract_keyspace {
  size_t size;
  size_t fast_size;
  size_t prefix_m;
  state root;
  monotonic_compressor compressor;
} abstract_keyspace;

/** @brief Keyspace where every stored key corresponds to a legal state. */
typedef struct compressed_keyspace {
  size_t size;
  size_t fast_size;
  size_t prefix_m;
  state root;
  monotonic_compressor compressor;

  tight_keyspace keyspace;
} compressed_keyspace;

/** @brief Symmetry-reduced keyspace of legal canonical states. */
typedef struct symmetric_keyspace {
  size_t size;
  size_t fast_size;
  size_t prefix_m;
  state root;
  monotonic_compressor compressor;

  symmetry symmetry;
} symmetric_keyspace;

/** @brief Function pointer type used to mark keys that should be retained. */
typedef bool (*indicator_f)(const size_t key);

/** @brief Create a tight keyspace helper for a given root state. */
tight_keyspace create_tight_keyspace(const state *root, const bool symmetric_threats);

/** @brief Convert a child state of the root to a dense tight-key index. */
size_t to_tight_key_fast(const tight_keyspace *tks, const state *s);

/** @brief Recover a simple state from a dense tight-key index. */
state from_tight_key_fast(const tight_keyspace *tks, size_t key);

/** @brief Release allocations owned by a tight keyspace. */
void free_tight_keyspace(tight_keyspace *tks);

/** @brief Construct a compressed keyspace that stores only legal game states. */
compressed_keyspace create_compressed_keyspace(const state *root);

/** @brief Build a compressor for a monotonic sequence with legal membership indicated by the second argument. */
monotonic_compressor create_monotonic_compressor(size_t num_keys, indicator_f indicator);

/** @brief Compress a key by skipping entries that were not indicated. */
size_t compress_key(const monotonic_compressor *mc, const size_t key);

/** @brief Decompress a key back into the original uncompressed space. */
size_t decompress_key(const monotonic_compressor *mc, const size_t compressed_key);

/** @brief Return true when the compressed key was present in the original sequence. */
bool has_key(const monotonic_compressor *mc, const size_t compressed_key);

/** @brief Release allocations owned by a monotonic compressor. */
void free_monotonic_compressor (monotonic_compressor *mc);

/** @brief Convert a child state of the root to its compressed legal-state index. */
size_t to_compressed_key(const compressed_keyspace *cks, const state *s);

/** @brief Recover a state from a compressed legal-state index. */
state from_compressed_key(const compressed_keyspace *cks, size_t key);

/** @brief Remap a tight key into the compressed keyspace. */
size_t remap_tight_key(const compressed_keyspace *cks, size_t key);

/** @brief Return true when the given fast key decodes to a legal compressed state. */
bool was_compressed_legal(const compressed_keyspace *cks, size_t key);

/** @brief Release allocations owned by a compressed keyspace. */
void free_compressed_keyspace(compressed_keyspace *cks);

/** @brief Construct a symmetry-reduced keyspace of legal canonical states. */
symmetric_keyspace create_symmetric_keyspace(const state *root);

/** @brief Convert a child state of the root to its canonical compressed index. */
size_t to_symmetric_key(const symmetric_keyspace *sks, const state *s);

/** @brief Recover a canonical state from its compressed index. */
state from_symmetric_key(const symmetric_keyspace *sks, size_t key);

/** @brief Release allocations owned by a symmetric keyspace. */
void free_symmetric_keyspace(symmetric_keyspace *sks);

/** @brief Return true when the given fast key decodes to a legal canonical state. */
bool was_symmetric_legal(const symmetric_keyspace *sks, size_t key);

/** @brief Remap a canonical fast key into the compressed canonical keyspace. */
size_t remap_fast_key(const symmetric_keyspace *sks, size_t key);

/** @brief Recover a canonical state from a fast key. */
state from_fast_key(const symmetric_keyspace *sks, size_t key);

/** @brief Convert a state directly to its canonical fast key. */
size_t to_fast_key(const symmetric_keyspace *sks, const state *s);
