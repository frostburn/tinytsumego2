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

void test_chains() {
  int num_chains = 0;
  stones_t *cs = chains(single(1, 0) | single(0, 1), &num_chains);
  assert(num_chains == 2);
  assert(cs[0] == single(0, 1));
  assert(cs[1] == single(1, 0));
}

int main() {
  test_rectangles();
  test_chains();
  return EXIT_SUCCESS;
}
