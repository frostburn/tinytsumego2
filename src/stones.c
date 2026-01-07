#include <stdio.h>
#include "tinytsumego2/stones.h"

void print_stones(const stones_t stones) {
  // Column headers
  printf(" ");
  for (int i = 0; i < WIDTH; i++) {
      printf(" %c", 'A' + i);
  }
  printf("\n");

  for (int i = 0; i < 64; i++) {
    // Row headers. Zero indexed from top to bottom
    if (i % V_SHIFT == 0) {
      printf("%d", i / V_SHIFT);
    }

    // Stone indicators
    if ((1ULL << i) & stones) {
      printf(" @");
    }
    else {
      printf(" .");
    }

    if (i % V_SHIFT == V_SHIFT - 1){
      printf("\n");
    }
  }
  printf("\n");
}

stones_t rectangle(const int width, const int height) {
  stones_t r = 0;
  for (int i = 0; i < width; ++i)
  {
    for (int j = 0; j < height; ++j)
    {
      r |= 1ULL << (i * H_SHIFT + j * V_SHIFT);
    }
  }
  return r;
}

stones_t single(int x, int y) {
  return 1ULL << (x * H_SHIFT + y * V_SHIFT);
}

stones_t pass() {
  return 0ULL;
}

int popcount(const stones_t stones) {
  return __builtin_popcountll(stones);
}

int clz(const stones_t stones) {
  return __builtin_clzll(stones);
}

stones_t liberties(const stones_t stones, const stones_t empty) {
  return (
    ((stones & WEST_BLOCK) << H_SHIFT) |
    ((stones >> H_SHIFT) & WEST_BLOCK) |
    (stones << V_SHIFT) |
    (stones >> V_SHIFT)
  ) & ~stones & empty;
}

stones_t flood(register stones_t source, register const stones_t target) {
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
