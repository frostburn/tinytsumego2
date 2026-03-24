#pragma once

#include "tinytsumego2/stones.h"

/**
 * @file stones16.h
 * @brief Bitboard primitives for the alternate 16x4 board layout.
 */

/** @brief Alternate board width in intersections. */
#define WIDTH_16 (16)
/** @brief Alternate board height in intersections. */
#define HEIGHT_16 (4)

/** @brief Horizontal single-step shift for 16x4 bitboards. */
#define H_SHIFT_16 (1ULL)
/** @brief Vertical single-step shift for 16x4 bitboards. */
#define V_SHIFT_16 (WIDTH_16)

/** @brief Bit mask for the north edge of the 16x4 board. */
#define NORTH_WALL_16 ((1ULL << WIDTH_16) - 1ULL)
/** @brief Bit mask for the west edge of the 16x4 board. */
#define WEST_WALL_16 (1ULL | (1ULL << WIDTH_16) | (1ULL << (2 * WIDTH_16)) | (1ULL << (3 * WIDTH_16)))
/** @brief Board mask used before shifting east or west on the 16x4 board. */
#define WEST_BLOCK_16 ((~WEST_WALL_16) >> H_SHIFT_16)

/** @brief Bit mask for the south edge of the 16x4 board. */
#define SOUTH_WALL_16 (NORTH_WALL_16 << (V_SHIFT_16 * 3))
/** @brief Bit mask for the east edge of the 16x4 board. */
#define EAST_WALL_16 (WEST_WALL_16 << 15)

/** @brief Bit mask for row 0 on the 16x4 board. */
#define HH0 (NORTH_WALL_16)
/** @brief Bit mask for row 1 on the 16x4 board. */
#define HH1 (NORTH_WALL_16 << V_SHIFT_16)
/** @brief Bit mask for row 2 on the 16x4 board. */
#define HH2 (NORTH_WALL_16 << (2 * V_SHIFT_16))
/** @brief Bit mask for row 3 on the 16x4 board. */
#define HH3 (NORTH_WALL_16 << (3 * V_SHIFT_16))

/** @brief Bit mask for column 0 on the 16x4 board. */
#define VV0 (WEST_WALL_16)
/** @brief Bit mask for column 1 on the 16x4 board. */
#define VV1 (WEST_WALL_16 << H_SHIFT_16)
/** @brief Bit mask for column 2 on the 16x4 board. */
#define VV2 (WEST_WALL_16 << (2 * H_SHIFT_16))
/** @brief Bit mask for column 3 on the 16x4 board. */
#define VV3 (WEST_WALL_16 << (3 * H_SHIFT_16))
/** @brief Bit mask for column 4 on the 16x4 board. */
#define VV4 (WEST_WALL_16 << (4 * H_SHIFT_16))
/** @brief Bit mask for column 5 on the 16x4 board. */
#define VV5 (WEST_WALL_16 << (5 * H_SHIFT_16))
/** @brief Bit mask for column 6 on the 16x4 board. */
#define VV6 (WEST_WALL_16 << (6 * H_SHIFT_16))
/** @brief Bit mask for column 7 on the 16x4 board. */
#define VV7 (WEST_WALL_16 << (7 * H_SHIFT_16))

/**
 * @brief Print a 16x4 bitboard as ASCII art.
 *
 * @param stones Bitboard to print.
 */
void print_stones_16(const stones_t stones);

/**
 * @brief Construct a filled rectangle in the upper-left corner of the 16x4 board.
 *
 * @param width Rectangle width in intersections.
 * @param height Rectangle height in intersections.
 * @return Bitboard containing the requested rectangle.
 */
stones_t rectangle_16(const int width, const int height);

/**
 * @brief Construct a single-stone bitboard on the 16x4 board.
 *
 * @param x Zero-based column index.
 * @param y Zero-based row index.
 * @return Bitboard with a single set bit at `(x, y)`.
 */
stones_t single_16(const int x, const int y);

/**
 * @brief Compute liberties for stones on the 16x4 board.
 *
 * @param stones Bitboard containing one or more stones.
 * @param empty Bitboard containing empty intersections.
 * @return Bitboard of liberties adjacent to `stones` and contained in `empty`.
 */
stones_t liberties_16(const stones_t stones, const stones_t empty);

/**
 * @brief Flood fill a 16x4 bitboard from an initial seed.
 *
 * @param source Seed bitboard.
 * @param target Region to flood through.
 * @return Connected subset of `target` reachable from `source`.
 */
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

/**
 * @brief Flood fill a 16x4 bitboard without first masking the seed.
 *
 * @param source Seed bitboard.
 * @param target Region to flood through.
 * @return Connected subset of `target` reachable from `source`.
 */
#ifdef NDEBUG
inline stones_t bleed_16(register stones_t source, register const stones_t target) {
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
stones_t bleed_16(register stones_t source, register const stones_t target);
#endif

/**
 * @brief Expand a 16x4 bitboard orthogonally by one step.
 *
 * @param stones Bitboard to expand.
 * @return Expanded bitboard.
 */
stones_t cross_16(const stones_t stones);

/**
 * @brief Expand a 16x4 bitboard orthogonally and diagonally by one step.
 *
 * @param stones Bitboard to expand.
 * @return Expanded bitboard.
 */
stones_t blob_16(stones_t stones);

/**
 * @brief Convert a single 16x4 bit to a column label.
 *
 * @param stone Bitboard containing a single stone or `pass()`.
 * @return `'A'` through `'P'`, or `'p'` for a pass.
 */
char column_of_16(const stones_t stone);

/**
 * @brief Convert a single 16x4 bit to a row label.
 *
 * @param stone Bitboard containing a single stone or `pass()`.
 * @return `'0'` through `'3'`, or `'s'` for a pass.
 */
char row_of_16(const stones_t stone);

/**
 * @brief Split a 16x4 bitboard into connected chains.
 *
 * @param stones Bitboard to decompose.
 * @param num_chains Output parameter receiving the number of chains.
 * @return Newly allocated array of chain bitboards. The caller must free it.
 */
stones_t *chains_16(stones_t stones, int *num_chains);

/**
 * @brief Split a 16x4 bitboard into one-bit stones.
 *
 * @param stones Bitboard to decompose.
 * @param num_dots Output parameter receiving the number of stones.
 * @return Newly allocated array of one-bit bitboards. The caller must free it.
 */
stones_t *dots_16(stones_t stones, int *num_dots);

/**
 * @brief Mirror a 16x4 bitboard across the horizontal axis.
 *
 * @param stones Bitboard to mirror.
 * @return Mirrored bitboard.
 */
stones_t stones_mirror_v_16(stones_t stones);

/**
 * @brief Mirror a 16x4 bitboard across the vertical axis.
 *
 * @param stones Bitboard to mirror.
 * @return Mirrored bitboard.
 */
stones_t stones_mirror_h_16(stones_t stones);

/**
 * @brief Translate a 16x4 bitboard to the upper-left corner.
 *
 * @param stones Bitboard to translate.
 * @return Snapped bitboard.
 */
stones_t stones_snap_16(stones_t stones);

/**
 * @brief Test whether all bits belong to one orthogonally connected chain.
 *
 * @param stones Bitboard to test.
 * @return True when the stones form one chain.
 */
bool is_contiguous_16(const stones_t stones);

/**
 * @brief Measure the occupied width of a 16x4 bitboard.
 *
 * @param stones Bitboard to measure.
 * @return Occupied width in intersections.
 */
int width_of_16(const stones_t stones);

/**
 * @brief Measure the occupied height of a 16x4 bitboard.
 *
 * @param stones Bitboard to measure.
 * @return Occupied height in intersections.
 */
int height_of_16(const stones_t stones);

/**
 * @brief Count empty columns to the west of a 16x4 bitboard.
 *
 * @param stones Bitboard to measure.
 * @return Number of empty columns to the west.
 */
int offset_h_16(stones_t stones);

/**
 * @brief Count empty rows to the north of a 16x4 bitboard.
 *
 * @param stones Bitboard to measure.
 * @return Number of empty rows to the north.
 */
int offset_v_16(stones_t stones);

/**
 * @brief Shift stones left by the given amount.
 *
 * @param stones Bitboard to shift.
 * @param amount Number of columns to shift west.
 * @return Shifted bitboard.
 */
stones_t move_west_16(stones_t stones, int amount);

/**
 * @brief Convert a single 16x4 stone back to coordinates or {-1, -1} for `pass()`.
 *
 * @param stone Bitboard containing a single stone or `pass()`.
 * @return Coordinates for `single_16(x, y)` or {-1, -1} for `pass()`.
 */
coordinates coords_of_16(const stones_t stone);
