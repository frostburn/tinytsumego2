#pragma once

#include <stdbool.h>
/*
 * Tsumego (go problems) are arranged inside a 9x7 goban (playing area).
 * Stones (playing pieces) are represented as bit boards (unsigned integers).
 * Every bit (power of two) signifies the presence or absence of a stone.
 */
#define WIDTH (9)
#define HEIGHT (7)

// Bit shifts associated with "physical" shifts of stones on the goban
#define H_SHIFT (1ULL)
#define V_SHIFT (WIDTH)
#define D_SHIFT (WIDTH - 1ULL)

// Bit boards associated with goban geometry
#define NORTH_WALL ((1ULL << WIDTH) - 1ULL)
#define WEST_WALL (0x40201008040201ULL)
#define WEST_BLOCK (0X3FDFEFF7FBFDFEFF)

// Horizontal strips
#define H0 (NORTH_WALL)
#define H1 (H0 << V_SHIFT)
#define H2 (H1 << V_SHIFT)
#define H3 (H2 << V_SHIFT)
#define H4 (H3 << V_SHIFT)
#define H5 (H4 << V_SHIFT)
#define H6 (H5 << V_SHIFT)
#define SOUTH_WALL (H6)

// Vertical strips
#define V0 (WEST_WALL)
#define V1 (V0 << H_SHIFT)
#define V2 (V1 << H_SHIFT)
#define V3 (V2 << H_SHIFT)
#define V4 (V3 << H_SHIFT)
#define V5 (V4 << H_SHIFT)
#define V6 (V5 << H_SHIFT)
#define V7 (V6 << H_SHIFT)
#define V8 (V7 << H_SHIFT)
#define EAST_WALL (V8)

// Last bit
#define LAST_STONE (1ULL << 63)

// 2x1 nubs
#define H_NUB (3ULL)
#define V_NUB (513ULL)

// Area that cannot be rotated or mirrored diagonally
#define EAST_STRIP (V7 | V8)

#define MAX_CHAINS (32)

// 64 bits of stones (1 bit outside the 9x7 rectangle)
typedef unsigned long long int stones_t;

// Print a bit board with "." for 0 bits and "@" for 1 bits. An extra row included for the 64th bit.
void print_stones(const stones_t stones);

// Return a rectangle of stones
stones_t rectangle(const int width, const int height);

// Return a single stone at the given coordinates
stones_t single(const int x, const int y);

// Return the zero bit board corresponding to a passing move
stones_t pass();

// Return the number of stones in the bit board
int popcount(const stones_t stones);

// Count empty spaces before the first stone in the bit board
int ctz(const stones_t stones);

// Count empty spaces after the last stone in the bit board
int clz(const stones_t stones);

// Return the bit board indicating the liberties of `stones` that lie in `empty` space
stones_t liberties(const stones_t stones, const stones_t empty);

// Flood fill `target` starting from `source` and return the contiguous chain of stones
stones_t flood(register stones_t source, register const stones_t target);

// Expand `stones` in a "cross" pattern
stones_t cross(const stones_t stones);

// Expand `stones` in a "blob" pattern
stones_t blob(stones_t stones);

// Indicate the column of a single stone: 'A' through 'I' or 'p' for pass()
char column_of(const stones_t stone);

// Indicate the row of a single stone: '0' through '6' or 's' for pass()
char row_of(const stones_t stone);

// Break `stones` into individual chains. Returns a dynamically allocated array. Stores the number of chains in the second argument.
stones_t *chains(stones_t stones, int *num_chains);

// Break `stones` into individual stone bits. Stores the number of bits in the second argument.
stones_t *dots(stones_t stones, int *num_dots);

// Mirror `stones` vertically
stones_t stones_mirror_v(stones_t stones);

// Mirror `stones` horizontally
stones_t stones_mirror_h(stones_t stones);

// Mirror `stones` diagonally
stones_t stones_mirror_d(stones_t stones);

// Snap `stones` to the upper left corner
stones_t stones_snap(stones_t stones);

// Return `true` if the stones form a single chain
bool is_contiguous(stones_t stones);

// Return how far east `stones` extend
int width_of(stones_t stones);

// Return how far south `stones` extend
int height_of(stones_t stones);
