#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/stones16.h"

void test_rectangles() {
  for (int i = 0; i <= 9; ++i) {
    for (int j = 0; j <= 7; ++j) {
      assert(popcount(rectangle(i, j)) == i * j);
    }
  }
}

void test_rectangles_16() {
  print_stones_16(rectangle_16(4, 3));
  for (int i = 0; i <= 16; ++i) {
    for (int j = 0; j <= 4; ++j) {
      assert(popcount(rectangle_16(i, j)) == i * j);
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

void test_width_of() {
  assert(width_of(0ULL) == 0);
  assert(width_of(1ULL) == 1);
  assert(width_of(5ULL << V_SHIFT) == 3);
}

void test_height_of() {
  assert(height_of(0ULL) == 0);
  assert(height_of(1ULL) == 1);
  assert(height_of(5ULL << V_SHIFT) == 2);
}

void test_wide_stones() {
  print_stones_16(WEST_WALL_16);
  print_stones(WEST_BLOCK);
  print_stones_16(WEST_BLOCK_16);
  stones_t w = WEST_WALL_16;
  w |= w << 1;
  stones_t w2 = w;
  w |= w << 2;
  stones_t w3 = w;
  w |= w << 4;

  w2 |= w2 << 4;
  w2 |= w2 << 8;

  w3 |= w3 << 8;
  print_stones_16(w);
  print_stones_16(w3);
  print_stones_16(w2);
  print_stones_16(w2 ^ (w2 << 1));

  printf("0x%llxULL, 0x%llxULL, 0x%llxULL, 0x%llxULL\n", w, w3, w2, w2 ^ (w2 << 1));

  stones_t s = 3498723948723746752ULL;
  print_stones_16(s);
  print_stones_16(stones_mirror_v_16(s));
  print_stones_16(stones_mirror_h_16(s));

  assert(stones_mirror_v_16(s) == 13240849900958396557ULL);
  assert(stones_mirror_h_16(s) == 12757615202855224301ULL);
}

int main() {
  test_rectangles();
  test_rectangles_16();
  test_chains();
  test_width_of();
  test_height_of();
  test_wide_stones();
  return EXIT_SUCCESS;
}
