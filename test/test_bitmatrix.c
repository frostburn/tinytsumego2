#include <assert.h>
#include "jkiss/jkiss.h"
#include "tinytsumego2/bitmatrix.h"

void test_bitmatrix() {
  int n = 30;
  int *coords = malloc(2 * n * sizeof(int));
  int width = 3 + (jrand() % 71);
  int height = 4 + (jrand() % 17);
  bitmatrix bm = create_bitmatrix(width, height);

  int *popcounts = calloc(height, sizeof(int));

  print_bitmatrix(&bm);

  for (int i = 0; i < n; ++i) {
    coords[2*i] = jrand() % width;
    coords[2*i + 1] = jrand() % height;
    printf("Setting %d, %d\n", coords[2*i], coords[2*i + 1]);
    bool already_there = false;
    for (int j = 0; j < i; ++j) {
      already_there = already_there || (coords[2*j] == coords[2*i] && coords[2*j + 1] == coords[2*i + 1]);
    }
    if (already_there) {
      assert(bitmatrix_get(&bm, coords[2*i], coords[2*i + 1]));
    } else {
      popcounts[coords[2*i + 1]]++;
      assert(!bitmatrix_get(&bm, coords[2*i], coords[2*i + 1]));
    }
    bitmatrix_set(&bm, coords[2*i], coords[2*i + 1]);
  }

  print_bitmatrix(&bm);

  for (int i = 0; i < n; ++i) {
    assert(bitmatrix_get(&bm, coords[2*i], coords[2*i + 1]));
  }

  for (int j = 0; j < height; ++j) {
    assert(popcounts[j] == bitmatrix_row_popcount(&bm, j));
  }

  for (int i = 0; i < n; ++i) {
    assert(bitmatrix_has_column(&bm, coords[2*i]));
  }

  int nuked = 0;
  for (int j = 0; j < height; ++j) {
    if (popcounts[j]) {
      bitmatrix_nuke_columns(&bm, j);
      nuked = j;
      break;
    }
  }

  printf("After nuking %d\n", nuked);
  print_bitmatrix(&bm);

  for (int i = 0; i < n; ++i) {
    if (coords[2*i + 1] == nuked) {
      assert(!bitmatrix_get(&bm, coords[2*i], coords[2*i + 1]));
    }
  }

  free(popcounts);
  free_bitmatrix(&bm);
}

int main() {
  jkiss_init();
  test_bitmatrix();
}
