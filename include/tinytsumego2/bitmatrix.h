#pragma once
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @file bitmatrix.h
 * @brief Dense rectangular matrix of bits used by helper algorithms.
 */

/**
 * @brief Heap-allocated rectangular matrix of zero/one values.
 */
typedef struct bitmatrix {
  /** @brief Number of logical columns. */
  int width;
  /** @brief Number of logical rows. */
  int height;
  /** @brief Number of storage cells per row. */
  size_t n_row_cells;
  /** @brief Packed matrix storage in row-major order. */
  unsigned int *data;
} bitmatrix;

/** @brief Number of bits stored in one backing cell. */
#define UINT_BITS (sizeof(unsigned int) * CHAR_BIT)

/** @brief Create a zero-initialized `width` by `height` bitmatrix. */
bitmatrix create_bitmatrix(int width, int height);

/** @brief Print the contents of a bitmatrix. */
void print_bitmatrix(bitmatrix *bm);

/** @brief Set the bit at `(x, y)` to one. */
void bitmatrix_set(bitmatrix *bm, int x, int y);

/** @brief Read the bit at `(x, y)`. */
bool bitmatrix_get(bitmatrix *bm, int x, int y);

/** @brief Count the number of set bits in row `y`. */
int bitmatrix_row_popcount(bitmatrix *bm, int y);

/** @brief Clear all columns whose bit is set in row `y`. */
void bitmatrix_nuke_columns(bitmatrix *bm, int y);

/** @brief Return true when column `x` contains at least one set bit. */
bool bitmatrix_has_column(bitmatrix *bm, int x);

/** @brief Release memory owned by a bitmatrix. */
void free_bitmatrix(bitmatrix *bm);
