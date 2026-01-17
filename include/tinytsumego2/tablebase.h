#pragma once
#include <stdint.h>
#include "tinytsumego2/state.h"

#define TABLE_WIDTH (3)
#define TABLE_HEIGHT (3)

// 3**(TABLE_WIDTH * TABLE_HEIGHT)
// #define TABLEBASE_SIZE (531441UL) // 3**12
#define TABLEBASE_SIZE (19683UL)

#define INVALID_KEY (SIZE_MAX)

// Convert a 4x3 corner tsumego into an enumerated index. Stores score change in the second argument.
size_t to_corner_tablebase_key(state *s, float *delta);

// Build a game state from an enumeration of every 4x3 corner tsumego
state from_corner_tablebase_key(size_t key);

// Build a game state from an enumeration of every 4x3 edge tsumego
state from_edge_tablebase_key(size_t key);

// TODO: Edge on along the horizontal edge if 4x4 tablebase is not happening

// Build a game state from an enumeration of every 4x3 center tsumego
state from_center_tablebase_key(size_t key);
