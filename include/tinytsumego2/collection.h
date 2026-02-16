#pragma once

#include <stdbool.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"

// A collection of go problems to show to end-users
// Normalized to the upper-left/north-western corner with the rest of the 19x19 goban empty

// GET api/tsumego/
// {
//    "collections": ["rectangle-six", ...]
// }

// GET api/tsumego/{collection: rectangle-six}/
// {
//    "title": "Rectangular Six in the Corner",
//    "root": {"visualArea": [511, 511, ...]},
//    "tsumegos": ["no-liberties", "one-liberty", ...]
// }

// GET api/tsumego/{collection: rectangle-six}/{tsumego: no-liberties}/
// {
//    "title": "Rectangular Six in the Corner",
//    "subtitle": "No Outside Liberties",
//    "state": {"visualArea": [511, 511, ...]},
//    "botToPlay": false
// }
//

// POST api/tsumego/{collection: rectangle-six}/
// In: {"state": {...}}
// Out: {"moves": [...], ...}

typedef struct tsumego {
  // URL-friendly name
  char *slug;
  // User-friendly name
  char *subtitle;
  // The game state must be enumerable from the collection root (enumeration may be abused for aesthetics)
  state state;
  // Allowing the book/computer to play first lets us catalogue classic "dead" shapes
  bool bot_to_play;
  // Value is used to verify collection generation (use {NAN, NAN} to skip)
  value value;
} tsumego;

typedef struct collection {
  // URL-friendly name
  char *slug;
  // User-friendly name
  char *title;
  // Dual-graph root in the upper-left corner. Solutions should be preserved if goban is extended to full 19x19.
  state root;
  // Sub-problems that can be reached from the root
  size_t num_tsumegos;
  tsumego *tsumegos;
} collection;

// Get an array of hard-coded collections of tsumegos
collection* get_collections(size_t *num_collections);
