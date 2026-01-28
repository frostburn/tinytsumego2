#pragma once
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Rectangular matrix of zeros and ones
typedef struct bitmatrix {
  int width;
  int height;
  size_t n_row_cells;
  unsigned int *data;
} bitmatrix;

// Number of bits in a bitmatrix cell
#define UINT_BITS (sizeof(unsigned int) * CHAR_BIT)

// Create a `width` x `height` bitmatrix
bitmatrix create_bitmatrix(int width, int height);

// Print the contents of the bitmatrix
void print_bitmatrix(bitmatrix *bm);

// Set a bit in the bitmatrix
void bitmatrix_set(bitmatrix *bm, int x, int y);

// Get a bit from the bitmatrix
bool bitmatrix_get(bitmatrix *bm, int x, int y);

// Calculate the number of non-zero bits in the given row
int bitmatrix_row_popcount(bitmatrix *bm, int y);

// Clear all columns where bits are set on the given row
void bitmatrix_nuke_columns(bitmatrix *bm, int y);

// Returns `true` if the given column has bits set. Otherwise returns `false`
bool bitmatrix_has_column(bitmatrix *bm, int x);

// Release resources associated with the bitmatrix
void free_bitmatrix(bitmatrix *bm);
