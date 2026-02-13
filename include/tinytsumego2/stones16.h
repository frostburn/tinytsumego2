#pragma once

#include "tinytsumego2/stones.h"
/*
 * Tsumego (go problems) are arranged inside a 16x4 goban (playing area).
 * Stones (playing pieces) are represented as bit boards (unsigned integers).
 * Every bit (power of two) signifies the presence or absence of a stone.
 */
#define WIDTH_16 (16)
#define HEIGHT_16 (4)

// Bit shifts associated with "physical" shifts of stones on the goban
#define H_SHIFT_16 (1ULL)
#define V_SHIFT_16 (WIDTH_16)

// Bit boards associated with goban geometry
#define NORTH_WALL_16 ((1ULL << WIDTH_16) - 1ULL)
#define WEST_WALL_16 (1ULL | (1ULL << WIDTH_16) | (1ULL << (2 * WIDTH_16)) | (1ULL << (3 * WIDTH_16)))
#define WEST_BLOCK_16 ((~WEST_WALL_16) >> H_SHIFT_16)

#define SOUTH_WALL_16 (NORTH_WALL_16 << (V_SHIFT_16 * 3))
#define EAST_WALL_16 (WEST_WALL_16 << 15)

// Print a bit board with "." for 0 bits and "@" for 1 bits
void print_stones_16(const stones_t stones);

// Return a rectangle of stones
stones_t rectangle_16(const int width, const int height);

// Return a single stone at the given coordinates
stones_t single_16(const int x, const int y);

// Return the bit board indicating the liberties of `stones` that lie in `empty` space
stones_t liberties_16(const stones_t stones, const stones_t empty);

// Flood fill `target` starting from `source` and return the contiguous chain of stones
#ifdef NDEBUG
inline stones_t flood_16(register stones_t source, register const stones_t target) {
  source &= target;
  register stones_t temp;
  do {
    temp = source;
    source |= (
      ((source & WEST_BLOCK_16) << H_SHIFT_16) |
      ((source >> H_SHIFT_16) & WEST_BLOCK_16) |
      (source << V_SHIFT_16) |
      (source >> V_SHIFT_16)
    ) & target;
  } while (temp != source);
  return source;
}
#else
stones_t flood_16(register stones_t source, register const stones_t target);
#endif

// Expand `stones` in a "cross" pattern
stones_t cross_16(const stones_t stones);

// Expand `stones` in a "blob" pattern
stones_t blob_16(stones_t stones);

// Indicate the column of a single stone: 'A' through 'P' or 'p' for pass()
char column_of_16(const stones_t stone);

// Indicate the row of a single stone: '0' through '3' or 's' for pass()
char row_of_16(const stones_t stone);

// Break `stones` into individual chains. Returns a dynamically allocated array. Stores the number of chains in the second argument.
stones_t *chains_16(stones_t stones, int *num_chains);

// Break `stones` into individual stone bits. Stores the number of bits in the second argument.
stones_t *dots_16(stones_t stones, int *num_dots);

// Mirror `stones` vertically
stones_t stones_mirror_v_16(stones_t stones);

// Mirror `stones` horizontally
stones_t stones_mirror_h_16(stones_t stones);

// Snap `stones` to the upper left corner
stones_t stones_snap_16(stones_t stones);

// Return `true` if the stones form a single chain
bool is_contiguous_16(const stones_t stones);

// Return how far east `stones` extend
int width_of_16(const stones_t stones);

// Return how far south `stones` extend
int height_of_16(const stones_t stones);

// Return how many empty columns there are to the west of `stones`
int offset_h_16(stones_t stones);

// Return how many empty rows there are to the north of `stones`
int offset_v_16(stones_t stones);

// Shift stones to the left by the given `amount`
stones_t move_west_16(stones_t stones, int amount);
