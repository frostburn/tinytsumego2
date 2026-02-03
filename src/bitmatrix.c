#include "tinytsumego2/bitmatrix.h"
#include "tinytsumego2/util.h"

bitmatrix create_bitmatrix(int width, int height) {
  bitmatrix result = (bitmatrix) {width, height, ceil_div(width, UINT_BITS), NULL};
  result.data = calloc(result.n_row_cells * ((size_t) height), sizeof(unsigned int));
  return result;
}

void print_bitmatrix(bitmatrix *bm) {
  for (int i = 0; i < bm->height; ++i) {
    for (size_t j = 0; j < bm->n_row_cells; ++j) {
      unsigned int cell = bm->data[i * bm->n_row_cells + j];
      for (unsigned int k = 0; k < UINT_BITS; ++k) {
        printf("%u", cell & 1);
        cell >>= 1;
      }
      printf(" ");
    }
    printf("\n");
  }
}

void bitmatrix_set(bitmatrix *bm, int x, int y) {
  bm->data[(x / UINT_BITS) + y * bm->n_row_cells] |= 1 << (x % UINT_BITS);
}

bool bitmatrix_get(bitmatrix *bm, int x, int y) {
  return bm->data[(x / UINT_BITS) + y * bm->n_row_cells] & (1 << (x % UINT_BITS));
}

int bitmatrix_row_popcount(bitmatrix *bm, int y) {
  int total = 0;
  for (size_t i = 0; i < bm->n_row_cells; ++i) {
    total += __builtin_popcount(bm->data[y * bm->n_row_cells + i]);
  }
  return total;
}

void bitmatrix_nuke_columns(bitmatrix *bm, int y) {
  for (size_t i = 0; i < bm->n_row_cells; ++i) {
    unsigned int mask = ~bm->data[y * bm->n_row_cells + i];
    for (size_t j = 0; j < (size_t) bm->height; ++j) {
      bm->data[j * bm->n_row_cells + i] &= mask;
    }
  }
}

bool bitmatrix_has_column(bitmatrix *bm, int x) {
  unsigned int mask = (1 << (x % UINT_BITS));
  size_t i = x / UINT_BITS;
  for (size_t j = 0; j < (size_t) bm->height; ++j) {
    if (bm->data[j * bm->n_row_cells + i] & mask) {
      return true;
    }
  }
  return false;
}

void free_bitmatrix(bitmatrix *bm) {
  free(bm->data);
  bm->width = 0;
  bm->height = 0;
  bm->n_row_cells = 0;
  bm->data = NULL;
}
