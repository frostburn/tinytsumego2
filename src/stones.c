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
    if (i % WIDTH == 0) {
      printf("%d", i / WIDTH);
    }

    // Stone indicators
    if ((1ULL << i) & stones) {
      printf(" @");
    }
    else {
      printf(" .");
    }

    if (i % WIDTH == WIDTH - 1){
      printf("\n");
    }
  }
  printf("\n");
}

stones_t rectangle(const int width, const int height) {
  if (!height) {
    return 0ULL;
  }
  stones_t r = (1ULL << width) - 1ULL;
  for (int j = 1; j < height; ++j) {
    r |= r << V_SHIFT;
  }
  return r;
}

stones_t single(const int x, const int y) {
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

stones_t cross(const stones_t stones) {
  return (
    ((stones & WEST_BLOCK) << H_SHIFT) |
    ((stones >> H_SHIFT) & WEST_BLOCK) |
    (stones << V_SHIFT) |
    (stones >> V_SHIFT) |
    stones
  );
}

stones_t blob(stones_t stones) {
  stones |= ((stones & WEST_BLOCK) << H_SHIFT) | ((stones >> H_SHIFT) & WEST_BLOCK);
  return stones | (stones << V_SHIFT) | (stones >> V_SHIFT);
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

  // Nub alignment on the goban
  // 0
  // 0
  // 1
  // 1
  // 2
  // 2
  // 3
  // 3
  for (stones_t nub = V_NUB; nub; nub <<= 2 * V_SHIFT) {
    stones_t chain = flood(nub, stones);
    if (chain) {
      result[(*num_chains)++] = chain;
      stones ^= chain;
    }
  }

  // . 0 0 1 1 2 2 3 3
  for (stones_t nub = H_NUB << 1; stones; nub <<= 2) {

    // . 0 0
    // . 1 1
    // . 2 2
    // . 3 3
    // . (etc.)
    for (stones_t n = nub; n; n <<= V_SHIFT) {
      stones_t chain = flood(n, stones);
      if (chain) {
        result[(*num_chains)++] = chain;
        stones ^= chain;
      }
    }
  }
  return realloc(result, (*num_chains) * sizeof(stones_t));
}

stones_t *dots(stones_t stones, int *num_dots) {
  stones_t *result = malloc(64 * sizeof(stones_t));
  *num_dots = 0;
  stones_t p = 1ULL;
  while (stones) {
    if (p & stones) {
      result[(*num_dots)++] = p;
      stones ^= p;
    }
    p <<= 1;
  }
  return realloc(result, (*num_dots) * sizeof(stones_t));
}

#define HB3 (H0 | H1 | H2)
#define HE7 (H0 | H4)
#define HC7 (H1 | H3 | H5)

stones_t stones_mirror_v(stones_t stones) {
  stones = ((stones >> (4 * V_SHIFT)) & HB3) | ((stones & HB3) << (4 * V_SHIFT)) | (stones & H3);
  return ((stones >> (2 * V_SHIFT)) & HE7) | ((stones & HE7) << (2 * V_SHIFT)) | (stones & HC7);
}

#define VB3 (V0 | V1 | V2)
#define VBC3 (V3 | V4 | V5)
#define VE9 (V0 | V3 | V6)
#define VC9 (V1 | V4 | V7)

stones_t stones_mirror_h(stones_t stones) {
  stones = ((stones >> 6) & VB3) | ((stones & VB3) << 6) | (stones & VBC3);
  return ((stones >> 2) & VE9) | ((stones & VE9) << 2) | (stones & VC9);
}

stones_t stones_mirror_d(stones_t stones) {
  return (
    (stones & 0x1004010040100401ULL) |
    ((stones & 0x8020080200802ULL) << D_SHIFT) |
    ((stones >> D_SHIFT) & 0x8020080200802ULL) |
    ((stones & 0x40100401004ULL) << (2 * D_SHIFT)) |
    ((stones >> (2 * D_SHIFT)) & 0x40100401004ULL) |
    ((stones & 0x200802008ULL) << (3 * D_SHIFT)) |
    ((stones >> (3 * D_SHIFT)) & 0x200802008ULL) |
    ((stones & 0x1004010ULL) << (4 * D_SHIFT)) |
    ((stones >> (4 * D_SHIFT)) & 0x1004010ULL) |
    ((stones & 0x8020ULL) << (5 * D_SHIFT)) |
    ((stones >> (5 * D_SHIFT)) & 0x8020ULL) |
    ((stones & 0x40ULL) << (6 * D_SHIFT)) |
    ((stones >> (6 * D_SHIFT)) & 0x40ULL)
  );
}

stones_t stones_snap(stones_t stones) {
  if (!stones) {
    return stones;
  }
  while (!(stones & NORTH_WALL)) {
    stones >>= V_SHIFT;
  }
  while (!(stones & WEST_WALL)) {
    stones >>= H_SHIFT;
  }
  return stones;
}

bool is_contiguous(const stones_t stones) {
  return flood(1ULL << ctz(stones), stones) == stones;
}

int width_of(const stones_t stones) {
  stones_t w = EAST_WALL;
  for (int result = WIDTH; result; result--) {
    if (stones & w) {
      return result;
    }
    w >>= H_SHIFT;
  }
  if (stones) {
    return 1;
  }
  return 0;
}

int height_of(const stones_t stones) {
  int result = HEIGHT;
  for (stones_t w = SOUTH_WALL; w; w >>= V_SHIFT) {
    if (stones & w) {
      return result;
    }
    result--;
  }
  if (stones) {
    return HEIGHT + 1;
  }
  return 0;
}

int offset_h(stones_t stones) {
  if (!stones) {
    return 0;
  }
  for (int result = 0; result < WIDTH - 1; ++result) {
    if (stones & WEST_WALL) {
      return result;
    }
    stones >>= H_SHIFT;
  }
  // Last bit
  return 0;
}

int offset_v(stones_t stones) {
  if (!stones) {
    return 0;
  }
  for(int result = 0;; result++) {
    if (stones & NORTH_WALL) {
      return result;
    }
    stones >>= V_SHIFT;
  }
  // Unreachable
  return 0;
}

#ifndef NDEBUG
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
#endif
