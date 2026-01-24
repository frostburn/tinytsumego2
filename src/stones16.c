#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/stones16.h"

void print_stones_16(const stones_t stones) {
  // Column headers
  printf(" ");
  for (int i = 0; i < WIDTH_16; i++) {
      printf(" %c", 'A' + i);
  }
  printf("\n");

  for (int i = 0; i < 64; i++) {
    // Row headers. Zero indexed from top to bottom
    if (i % WIDTH_16 == 0) {
      printf("%d", i / WIDTH_16);
    }

    // Stone indicators
    if ((1ULL << i) & stones) {
      printf(" @");
    }
    else {
      printf(" .");
    }

    if (i % WIDTH_16 == WIDTH_16 - 1){
      printf("\n");
    }
  }
  printf("\n");
}

stones_t rectangle_16(const int width, const int height) {
  if (!height) {
    return 0ULL;
  }
  stones_t r = (1ULL << width) - 1ULL;
  for (int j = 1; j < height; ++j) {
    r |= r << V_SHIFT_16;
  }
  return r;
}

stones_t single_16(const int x, const int y) {
  return 1ULL << (x * H_SHIFT_16 + y * V_SHIFT_16);
}

stones_t liberties_16(const stones_t stones, const stones_t empty) {
  return (
    ((stones & WEST_BLOCK_16) << H_SHIFT_16) |
    ((stones >> H_SHIFT_16) & WEST_BLOCK_16) |
    (stones << V_SHIFT_16) |
    (stones >> V_SHIFT_16)
  ) & ~stones & empty;
}

stones_t flood_16(register stones_t source, register const stones_t target) {
  source &= target;
  register stones_t temp;
  do {
    temp = source;
    source |= (
      ((source & WEST_BLOCK_16) << H_SHIFT_16) |
      ((source >> H_SHIFT_16) & WEST_BLOCK_16) |
      (source << V_SHIFT_16) |
      (source >> V_SHIFT_16)
    ) & target;
  } while (temp != source);
  return source;
}

stones_t cross_16(const stones_t stones) {
  return (
    ((stones & WEST_BLOCK_16) << H_SHIFT_16) |
    ((stones >> H_SHIFT_16) & WEST_BLOCK_16) |
    (stones << V_SHIFT_16) |
    (stones >> V_SHIFT_16) |
    stones
  );
}

stones_t blob_16(stones_t stones) {
  stones |= ((stones & WEST_BLOCK_16) << H_SHIFT_16) | ((stones >> H_SHIFT_16) & WEST_BLOCK_16);
  return stones | (stones << V_SHIFT_16) | (stones >> V_SHIFT_16);
}

char column_of_16(const stones_t stone) {
  if (!stone) {
    return 'p';
  }
  return 'A' + (ctz(stone) % WIDTH_16);
}

char row_of_16(const stones_t stone) {
  if (!stone) {
    return 's';
  }
  return '0' + (ctz(stone) / WIDTH_16);
}

stones_t *chains_16(stones_t stones, int *num_chains) {
  stones_t *result = malloc(MAX_CHAINS * sizeof(stones_t));
  *num_chains = 0;

  // Nub alignment on the goban
  // 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7
  for (stones_t nub = H_NUB; stones; nub <<= 2) {

    // 0 0
    // 1 1
    // 2 2
    // 3 3
    for (stones_t n = nub; n; n <<= V_SHIFT_16) {
      stones_t chain = flood_16(n, stones);
      if (chain) {
        result[(*num_chains)++] = chain;
        stones ^= chain;
      }
    }
  }
  return realloc(result, (*num_chains) * sizeof(stones_t));
}

#define HH0 (NORTH_WALL_16)
#define HH1 (NORTH_WALL_16 << V_SHIFT_16)
#define HH2 (NORTH_WALL_16 << (2 * V_SHIFT_16))
#define HH3 (NORTH_WALL_16 << (3 * V_SHIFT_16))

stones_t stones_mirror_v_16(stones_t stones) {
  return (
    ((stones & HH0) << (3 * V_SHIFT_16)) |
    ((stones & HH1) << V_SHIFT_16) |
    ((stones & HH2) >> V_SHIFT_16) |
    ((stones & HH3) >> (3 * V_SHIFT_16))
  );
}

#define VVA (0xff00ff00ff00ffULL)
#define VVB (0xf0f0f0f0f0f0f0fULL)
#define VVC (0x3333333333333333ULL)
#define VVD (0x5555555555555555ULL)

stones_t stones_mirror_h_16(stones_t stones) {
  stones = ((stones >> 8) & VVA) | ((stones & VVA) << 8);
  stones = ((stones >> 4) & VVB) | ((stones & VVB) << 4);
  stones = ((stones >> 2) & VVC) | ((stones & VVC) << 2);
  return ((stones >> 1) & VVD) | ((stones & VVD) << 1);
}

stones_t stones_snap_16(stones_t stones) {
  if (!stones) {
    return stones;
  }
  while (!(stones & NORTH_WALL_16)) {
    stones >>= V_SHIFT_16;
  }
  while (!(stones & WEST_WALL_16)) {
    stones >>= H_SHIFT_16;
  }
  return stones;
}

bool is_contiguous_16(const stones_t stones) {
  return flood_16(1ULL << ctz(stones), stones) == stones;
}

int width_of_16(const stones_t stones) {
  stones_t w = EAST_WALL_16;
  for (int result = WIDTH_16; result; result--) {
    if (stones & w) {
      return result;
    }
    w >>= H_SHIFT_16;
  }
  return 0;
}

int height_of_16(const stones_t stones) {
  int result = HEIGHT_16;
  for (stones_t w = SOUTH_WALL_16; w; w >>= V_SHIFT_16) {
    if (stones & w) {
      return result;
    }
    result--;
  }
  return 0;
}
