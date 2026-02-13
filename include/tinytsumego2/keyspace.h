#pragma once

#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

// Ternary conversion utility
typedef struct tritter {
  size_t m;
  stones_t shift;
  stones_t mask;
} tritter;

// Auxilary struct to help with tight key generation
typedef struct tight_keyspace {
  state root;
  size_t size;
  bool symmetric_threats;
  size_t ko_m;
  int num_tritters;
  tritter *tritters;
  size_t black_external_m;
  size_t white_external_m;

  size_t prefix_m;
  state *prefixes;
  int num_blocks;
  stones_t **black_blocks;
  stones_t **white_blocks;
} tight_keyspace;

// Compress monotonically increasing array of integers
typedef struct monotonic_compressor {
  size_t num_checkpoints;
  size_t *checkpoints;
  size_t uncompressed_size;
  unsigned char *deltas;
  size_t size;
  double factor;
} monotonic_compressor;

// Keyspace where every indexed state is legal
typedef struct compressed_keyspace {
  tight_keyspace keyspace;
  monotonic_compressor compressor;
  size_t prefix_m;
  size_t size;
} compressed_keyspace;

// Function pointer type for flagging legal keys
typedef bool (*indicator_f)(const size_t key);

// Create a keyspace helper for a root state
tight_keyspace create_tight_keyspace(const state *root, const bool symmetric_threats);

// Convert a child state of the original root state to a unique index
size_t to_tight_key_fast(const tight_keyspace *tks, const state *s);

// Recover a simple state from its unique index
state from_tight_key_fast(const tight_keyspace *tks, size_t key);

// Release associated resources
void free_tight_keyspace(tight_keyspace *tks);

// Construct a keyspace where every key corresponds to a legal game state
compressed_keyspace create_compressed_keyspace(const state *root);

// Create a monotonic sequence compressor
monotonic_compressor create_monotonic_compressor(size_t num_keys, indicator_f indicator);

// Compress an integer by skipping entries originally not indicated
size_t compress_key(const monotonic_compressor *mc, const size_t key);

// Decompress an integer (Warning: Slow)
size_t decompress_key(const monotonic_compressor *mc, const size_t compressed_key);

// Test if the key was originally indicated
bool has_key(const monotonic_compressor *mc, const size_t compressed_key);

// Release associated resources
void free_monotonic_compressor (monotonic_compressor *mc);

// Convert a child state of the original root state to a unique index
size_t to_compressed_key(const compressed_keyspace *cks, const state *s);

// Recover a simple state from its unique index (Warning: Slow)
state from_compressed_key(const compressed_keyspace *cks, size_t key);

// Remap tight keyspace element to the compressed keyspace
size_t remap_tight_key(const compressed_keyspace *cks, size_t key);

// Test if `from_tight_key_fast(&(cks->keyspace), key)` results in a legal state
bool was_legal(const compressed_keyspace *cks, size_t key);

// Release associated resources
void free_compressed_keyspace(compressed_keyspace *cks);
