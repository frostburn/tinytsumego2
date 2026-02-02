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
  bool white_to_play;
  size_t ko_m;
  int num_tritters;
  tritter *tritters;
  size_t player_external_m;
  size_t opponent_external_m;
} tight_keyspace;

// Create a keyspace helper for a root state
tight_keyspace create_tight_keyspace(const state *root);

// Convert a child state of the original root state to a unique index
size_t to_tight_key_fast(const tight_keyspace *tks, const state *s);

// Release associated resources
void free_tight_keyspace(tight_keyspace *tks);
