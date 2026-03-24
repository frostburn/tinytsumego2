#pragma once

#include <stdbool.h>

/**
 * @file stones.h
 * @brief Bitboard primitives for the default 9x7 tinytsumego board.
 *
 * The engine models stones, board geometry, and move coordinates as 64-bit
 * bitboards. Only the upper-left 9x7 rectangle is part of the playable area;
 * the extra bit dangles on the eighth line so the representation fits inside a
 * native unsigned 64-bit integer.
 */

/** @brief Default board width in intersections. */
#define WIDTH (9)
/** @brief Default board height in intersections. */
#define HEIGHT (7)

/** @brief Bit shift corresponding to a one-column horizontal move. */
#define H_SHIFT (1ULL)
/** @brief Bit shift corresponding to a one-row vertical move. */
#define V_SHIFT (WIDTH)
/** @brief Bit shift corresponding to a north-east / south-west diagonal step. */
#define D_SHIFT (WIDTH - 1ULL)

/** @brief Bit mask for the north edge of the board. */
#define NORTH_WALL ((1ULL << WIDTH) - 1ULL)
/** @brief Bit mask for the west edge of the board. */
#define WEST_WALL (0x40201008040201ULL)
/** @brief Board mask used before shifting east or west. */
#define WEST_BLOCK (0X3FDFEFF7FBFDFEFF)

/** @brief Bit mask for row 0. */
#define H0 (NORTH_WALL)
/** @brief Bit mask for row 1. */
#define H1 (H0 << V_SHIFT)
/** @brief Bit mask for row 2. */
#define H2 (H1 << V_SHIFT)
/** @brief Bit mask for row 3. */
#define H3 (H2 << V_SHIFT)
/** @brief Bit mask for row 4. */
#define H4 (H3 << V_SHIFT)
/** @brief Bit mask for row 5. */
#define H5 (H4 << V_SHIFT)
/** @brief Bit mask for row 6. */
#define H6 (H5 << V_SHIFT)
/** @brief Bit mask for the south edge of the board. */
#define SOUTH_WALL (H6)

/** @brief Bit mask for column 0. */
#define V0 (WEST_WALL)
/** @brief Bit mask for column 1. */
#define V1 (V0 << H_SHIFT)
/** @brief Bit mask for column 2. */
#define V2 (V1 << H_SHIFT)
/** @brief Bit mask for column 3. */
#define V3 (V2 << H_SHIFT)
/** @brief Bit mask for column 4. */
#define V4 (V3 << H_SHIFT)
/** @brief Bit mask for column 5. */
#define V5 (V4 << H_SHIFT)
/** @brief Bit mask for column 6. */
#define V6 (V5 << H_SHIFT)
/** @brief Bit mask for column 7. */
#define V7 (V6 << H_SHIFT)
/** @brief Bit mask for column 8. */
#define V8 (V7 << H_SHIFT)
/** @brief Bit mask for the east edge of the board. */
#define EAST_WALL (V8)

/** @brief Bit mask for the short north-west / south-east diagonal 0. */
#define D0 (0x1004010040100401ULL)
/** @brief Bit mask for diagonal 1. */
#define D1 (0x8020080200802ULL)
/** @brief Bit mask for diagonal 2. */
#define D2 (0x40100401004ULL)
/** @brief Bit mask for diagonal 3. */
#define D3 (0x200802008ULL)
/** @brief Bit mask for diagonal 4. */
#define D4 (0x1004010ULL)
/** @brief Bit mask for diagonal 5. */
#define D5 (0x8020ULL)
/** @brief Bit mask for diagonal 6. */
#define D6 (0x40ULL)

/** @brief The spare 64th bit outside the 9x7 rectangle. */
#define LAST_STONE (1ULL << 63)

/** @brief Horizontal 2x1 block anchored in the upper-left corner. */
#define H_NUB (3ULL)
/** @brief Vertical 2x1 block anchored in the upper-left corner. */
#define V_NUB (513ULL)

/** @brief Area that prevents diagonal symmetry on the default board. */
#define EAST_STRIP (V7 | V8)

/** @brief Maximum number of connected chains possible in a bitboard. */
#define MAX_CHAINS (32)

/** @brief Packed bitboard representation for stones and board masks. */
typedef unsigned long long int stones_t;

/**
 * @brief Integer board coordinates.
 *
 * The pair corresponds to the arguments accepted by single() or the values
 * returned by `coords_of()`. A passing move is represented as {-1, -1}.
 */
typedef struct coordinates {
  int x;
  int y;
} coordinates;

/**
 * @brief Print a bitboard as ASCII art.
 *
 * Prints `.` for cleared bits and `@` for set bits. An extra row is included
 * to display the spare 64th bit.
 *
 * @param stones Bitboard to print.
 */
void print_stones(const stones_t stones);

/**
 * @brief Construct a filled rectangle in the upper-left corner.
 *
 * @param width Rectangle width in intersections.
 * @param height Rectangle height in intersections.
 * @return Bitboard containing the requested rectangle.
 */
stones_t rectangle(const int width, const int height);

/**
 * @brief Construct a bitboard containing a single point.
 *
 * @param x Zero-based column index.
 * @param y Zero-based row index.
 * @return Bitboard with a single set bit at `(x, y)`.
 */
stones_t single(const int x, const int y);

/**
 * @brief Return the bitboard used to represent a pass.
 *
 * @return Zero bitboard.
 */
stones_t pass();

/** @brief Count the number of set bits in a bitboard. */
int popcount(const stones_t stones);

/** @brief Count trailing zero bits before the first set bit. */
int ctz(const stones_t stones);

/** @brief Count leading zero bits after the last set bit. */
int clz(const stones_t stones);

/**
 * @brief Compute liberties for stones inside a given empty region.
 *
 * @param stones Bitboard containing one or more stones.
 * @param empty Bitboard containing empty intersections.
 * @return Bitboard of liberties adjacent to `stones` and contained in `empty`.
 */
stones_t liberties(const stones_t stones, const stones_t empty);

/**
 * @brief Flood fill a target bitboard from an initial seed.
 *
 * In release builds this is defined inline for performance. The `source`
 * bitboard is first masked by `target` so out-of-target seed bits are ignored.
 *
 * @param source Seed bitboard.
 * @param target Region to flood through.
 * @return Connected subset of `target` reachable from `source`.
 */
#ifdef NDEBUG
inline stones_t flood(register stones_t source, register const stones_t target) {
  source &= target;
  register stones_t temp;
  do {
    temp = source;
    source |= (
      ((source & WEST_BLOCK) << H_SHIFT) |
      ((source >> H_SHIFT) & WEST_BLOCK) |
      (source << V_SHIFT) |
      (source >> V_SHIFT)
    ) & target;
  } while (temp != source);
  return source;
}
#else
stones_t flood(register stones_t source, register const stones_t target);
#endif

/**
 * @brief Flood fill a target bitboard without first masking the seed.
 *
 * @param source Seed bitboard.
 * @param target Region to flood through.
 * @return Connected subset of `target` reachable from `source`.
 */
#ifdef NDEBUG
inline stones_t bleed(register stones_t source, register const stones_t target) {
  register stones_t temp;
  do {
    temp = source;
    source |= (
      ((source & WEST_BLOCK) << H_SHIFT) |
      ((source >> H_SHIFT) & WEST_BLOCK) |
      (source << V_SHIFT) |
      (source >> V_SHIFT)
    ) & target;
  } while (temp != source);
  return source;
}
#else
stones_t bleed(register stones_t source, register const stones_t target);
#endif

/** @brief Expand a bitboard orthogonally by one step. */
stones_t cross(const stones_t stones);

/** @brief Expand a bitboard orthogonally and diagonally by one step. */
stones_t blob(stones_t stones);

/**
 * @brief Convert a single-stone bitboard to a column label.
 *
 * @param stone Bitboard containing a single stone or `pass()`.
 * @return `'A'` through `'I'`, or `'p'` for a pass.
 */
char column_of(const stones_t stone);

/**
 * @brief Convert a single-stone bitboard to a row label.
 *
 * @param stone Bitboard containing a single stone or `pass()`.
 * @return `'0'` through `'6'`, or `'s'` for a pass.
 */
char row_of(const stones_t stone);

/** @brief Function pointer type for coordinate-label helpers. */
typedef char (*coord_f)(const stones_t stone);

/**
 * @brief Convert a single-stone bitboard back to integer coordinates.
 *
 * @param stone Bitboard containing a single stone or `pass()`.
 * @return Coordinates for `single(x, y)` or {-1, -1} for `pass()`.
 */
coordinates coords_of(const stones_t stone);

/**
 * @brief Split a bitboard into connected chains.
 *
 * @param stones Bitboard to decompose.
 * @param num_chains Output parameter receiving the number of chains.
 * @return Newly allocated array of chain bitboards. The caller must free it.
 */
stones_t *chains(stones_t stones, int *num_chains);

/**
 * @brief Split a bitboard into individual one-bit stones.
 *
 * @param stones Bitboard to decompose.
 * @param num_dots Output parameter receiving the number of stones.
 * @return Newly allocated array of one-bit bitboards. The caller must free it.
 */
stones_t *dots(stones_t stones, int *num_dots);

/** @brief Mirror a bitboard across the horizontal axis. */
stones_t stones_mirror_v(stones_t stones);

/** @brief Mirror a bitboard across the vertical axis. */
stones_t stones_mirror_h(stones_t stones);

/** @brief Mirror a bitboard across the main diagonal. */
stones_t stones_mirror_d(stones_t stones);

/** @brief Translate a bitboard to the upper-left corner without changing shape. */
stones_t stones_snap(stones_t stones);

/** @brief Test whether all stones belong to one orthogonally connected chain. */
bool is_contiguous(const stones_t stones);

/** @brief Measure the occupied width of a bitboard. */
int width_of(const stones_t stones);

/** @brief Measure the occupied height of a bitboard. */
int height_of(const stones_t stones);

/** @brief Count empty columns to the west of a bitboard. */
int offset_h(stones_t stones);

/** @brief Count empty rows to the north of a bitboard. */
int offset_v(stones_t stones);
