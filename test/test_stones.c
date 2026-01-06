#include <assert.h>
#include <stdlib.h>
#include "tinytsumego2/stones.h"

void test_rectangles() {
  for (int i = 1; i <= 9; ++i) {
    for (int j = 1; j <= 7; ++j) {
      assert(popcount(rectangle(i, j)) == i * j);
    }
  }
}

int main() {
  test_rectangles();
  return EXIT_SUCCESS;
}
