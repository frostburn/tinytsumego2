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

// Create a keyspace helper for a root state
tight_keyspace create_tight_keyspace(const state *root, const bool symmetric_threats);

// Convert a child state of the original root state to a unique index
size_t to_tight_key_fast(const tight_keyspace *tks, const state *s);

// Recover a simple state from its unique index
state from_tight_key_fast(const tight_keyspace *tks, size_t key);

// Release associated resources
void free_tight_keyspace(tight_keyspace *tks);
