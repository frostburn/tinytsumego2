#include <stdio.h>
#include <stdlib.h>
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

int ctz(const stones_t stones) {
  return __builtin_ctzll(stones);
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

stones_t cross(const stones_t stones) {
  return (
    ((stones & WEST_BLOCK) << H_SHIFT) |
    ((stones >> H_SHIFT) & WEST_BLOCK) |
    (stones << V_SHIFT) |
    (stones >> V_SHIFT) |
    stones
  );
}

char column_of(const stones_t stone) {
  if (!stone) {
    return 'p';
  }
  return 'A' + (ctz(stone) % WIDTH);
}

char row_of(const stones_t stone) {
  if (!stone) {
    return 's';
  }
  return '0' + (ctz(stone) / WIDTH);
}

stones_t *chains(stones_t stones, int *num_chains) {
  stones_t *result = malloc(MAX_CHAINS * sizeof(stones_t));
  *num_chains = 0;
  // TODO: Pre-calculate nubs
  for (int i = 0; i < HEIGHT; i += 2) {
    stones_t chain = flood(V_NUB << (i * V_SHIFT), stones);
    if (chain) {
      result[(*num_chains)++] = chain;
      stones ^= chain;
    }
  }
  for (int i = 1; (i < WIDTH) && stones; i += 2) {
    for (int j = 0; j < HEIGHT; ++j) {
      stones_t chain = flood(H_NUB << (i + j * V_SHIFT), stones);
      if (chain) {
        result[(*num_chains)++] = chain;
        stones ^= chain;
      }
    }
  }
  return realloc(result, (*num_chains) * sizeof(stones_t));
}
