#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/stones.h"

// Enumerate all contiguous eyespace shapes for tabulation purposes

// Size 1
// .

// Size 2
// ..

#define MAX_SIZE (11)
#define VERBOSE

stones_t snap(stones_t stones) {
  if (!stones) {
    return 0ULL;
  }
  while (!(stones & WEST_WALL)) {
    stones >>= 1;
  }
  while (!(stones & NORTH_WALL)) {
    stones >>= V_SHIFT;
  }
  return stones;
}

stones_t canonize(stones_t stones) {
  stones = snap(stones);
  stones_t c = snap(stones_mirror_h(stones));
  if (c < stones) {
    stones = c;
  }
  c = snap(stones_mirror_v(c));
  if (c < stones) {
    stones = c;
  }
  c = snap(stones_mirror_h(c));
  if (c < stones) {
    stones = c;
  }

  if (stones & EAST_STRIP) {
    return stones;
  }

  c = snap(stones_mirror_d(c));
  if (c < stones) {
    stones = c;
  }
  c = snap(stones_mirror_h(c));
  if (c < stones) {
    stones = c;
  }
  c = snap(stones_mirror_v(c));
  if (c < stones) {
    stones = c;
  }
  c = snap(stones_mirror_h(c));
  if (c < stones) {
    stones = c;
  }

  return stones;
}

static size_t EXPECTED_COUNTS[] = {
  2,
  5,
  12,
  35,
  108,
  369,
  1285,
  4655 - 1, // The straight decomino doesn't fit
  17073 - 10, // The 10 extensions of the straight decomino don't fit
  63600,
  238591,
  901971,
  3426576,
  13079255,
  50107909,
  192622052,
  742624232,
  2870671950,
  11123060678,
  43191857688,
  168047007728,
  654999700403,
  2557227044764,
  9999088822075,
  39153010938487,
  153511100594603,
};

int main() {
  stones_t **polyominoes_by_size = malloc((MAX_SIZE - 2) * sizeof(stones_t*));
  polyominoes_by_size[0] = malloc(2 * sizeof(stones_t));
  size_t *polyomino_counts = (size_t*) calloc(MAX_SIZE - 2, sizeof(size_t));

  void expand_polyomino(stones_t polyomino) {
    size_t index = popcount(polyomino) - 2;

    int num_bits = 0;
    stones_t *bits = dots(cross(polyomino) ^ polyomino, &num_bits);
    for (int i = 0; i < num_bits; ++i) {
      stones_t candidate = canonize(polyomino | bits[i]);
      bool novel = true;
      for (size_t j = 0; j < polyomino_counts[index]; ++j) {
        if (polyominoes_by_size[index][j] == candidate) {
          novel = false;
          break;
        }
      }
      if (novel) {
        polyominoes_by_size[index][polyomino_counts[index]++] = candidate;
        #ifdef VERBOSE
          print_stones(candidate);
        #endif
      }
    }
  }

  expand_polyomino(3ULL << (1 + V_SHIFT));
  for (size_t size = 3; size < MAX_SIZE; ++size) {
    polyominoes_by_size[size - 2] = malloc(20000 * sizeof(stones_t));
    for (size_t i = 0; i < polyomino_counts[size - 3]; ++i) {
      stones_t polyomino = polyominoes_by_size[size - 3][i];
      expand_polyomino(polyomino);
      if (!(polyomino & EAST_WALL)) {
        expand_polyomino(polyomino << 1);
        if (!(polyomino & SOUTH_WALL)) {
          expand_polyomino(polyomino << (1 + V_SHIFT));
        }
      }
      if (!(polyomino & SOUTH_WALL)) {
        expand_polyomino(polyomino << V_SHIFT);
      }
    }
    polyominoes_by_size[size - 2] = realloc(polyominoes_by_size[size - 2], polyomino_counts[size - 2] * sizeof(stones_t));
  }

  for (size_t size = 3; size <= MAX_SIZE; ++size) {
    printf("%zu polyominoes of size %zu\n", polyomino_counts[size - 3], size);
    assert(polyomino_counts[size - 3] == EXPECTED_COUNTS[size - 3]);
  }

  return EXIT_SUCCESS;
}
