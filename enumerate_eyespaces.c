#include "tinytsumego2/stones.h"
#include <stdio.h>
#include <stdlib.h>

// Enumerate all contiguous eyespace shapes for tabulation purposes

// Size 1
// .

// Size 2
// ..

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

#define MAX_SIZE (6)

int main() {
  stones_t **eyespaces_by_size = malloc((MAX_SIZE - 2) * sizeof(stones_t*));
  eyespaces_by_size[0] = malloc(2 * sizeof(stones_t));
  size_t *eyespace_counts = (size_t*) calloc(MAX_SIZE - 2, sizeof(size_t));

  void expand_eyespace(stones_t eyespace) {
    size_t index = popcount(eyespace) - 2;
    eyespace = eyespace << (1 + V_SHIFT);

    int num_bits = 0;
    stones_t *bits = dots(cross(eyespace) ^ eyespace, &num_bits);
    for (int i = 0; i < num_bits; ++i) {
      stones_t candidate = canonize(eyespace | bits[i]);
      bool novel = true;
      for (size_t j = 0; j < eyespace_counts[index]; ++j) {
        if (eyespaces_by_size[index][j] == candidate) {
          novel = false;
          break;
        }
      }
      if (novel) {
        eyespaces_by_size[index][eyespace_counts[index]++] = candidate;
        print_stones(candidate);
      }
    }
  }

  expand_eyespace(3ULL);
  for (size_t size = 3; size < MAX_SIZE; ++size) {
    eyespaces_by_size[size - 2] = malloc(1000 * sizeof(stones_t));
    for (size_t i = 0; i < eyespace_counts[size - 3]; ++i) {
      expand_eyespace(eyespaces_by_size[size - 3][i]);
    }
    eyespaces_by_size[size - 2] = realloc(eyespaces_by_size[size - 2], eyespace_counts[size - 2] * sizeof(stones_t));
  }

  for (size_t size = 3; size <= MAX_SIZE; ++size) {
    printf("%zu eyespaces of size %zu\n", eyespace_counts[size - 3], size);
  }

  return EXIT_SUCCESS;
}
