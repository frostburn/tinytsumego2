#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/stones.h"

// Enumerate all contiguous eyespace shapes for tabulation purposes

// Size 1
// .

// Size 2
// ..

// Size 11 (too wide to fit)
// . . . . . . . . . . .

// . . . . . . . . . .
// .

// . . . . . . . . . .
//   .

// . . . . . . . . . .
//     .

// . . . . . . . . . .
//       .

// . . . . . . . . . .
//         .

//   . . . . . . . . .
// . .

//     . . . . . . . .
// . . .

//       . . . . . . .
// . . . .

//         . . . . . .
// . . . . .

#define MAX_SIZE (11)
// #define VERBOSE

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

bool is_whole(stones_t stones) {
  stones_t edges = (NORTH_WALL | WEST_WALL | EAST_WALL | SOUTH_WALL) & ~stones;
  stones_t negative_space = flood(edges, ~stones);
  return stones == ~negative_space;
}

// Assumes canonized input
bool is_corner_valid(stones_t stones) {
  stones_t negative_space = flood(single(WIDTH - 1, HEIGHT - 1), ~stones);
  if (stones == ~negative_space) {
    return true;
  }

  stones = snap(stones_mirror_h(stones));
  negative_space = flood(single(WIDTH - 1, HEIGHT - 1), ~stones);
  if (stones == ~negative_space) {
    return true;
  }

  stones = snap(stones_mirror_v(stones));
  negative_space = flood(single(WIDTH - 1, HEIGHT - 1), ~stones);
  return stones == ~negative_space;
}

// https://oeis.org/A000105
static size_t EXPECTED_COUNTS[] = {
  2,
  5,
  12,
  35,
  108,
  369,
  1285,
  4655 - 1, // The straight decomino doesn't fit
  17073 - 10, // See missing shapes above
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
      if (bits[i] == 1ULL << 63) {
        continue;
      }
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
    size_t num_whole = 0;
    size_t num_center_valid = 0;
    size_t num_edge_valid = 0;
    size_t num_corner_valid = 0;
    for (size_t i = 0; i < polyomino_counts[size - 3]; ++i) {
      stones_t polyomino = polyominoes_by_size[size - 3][i];
      if (is_whole(polyomino)) {
        num_whole++;
        // Canonization forces it to touch north and west

        // Can it be moved off the walls without touching others
        if (!(polyomino & (H5 | V7))) {
          num_center_valid++;
        }

        // Can it be moved off a wall (and is not already touching south or east)
        if (!(polyomino & (H6 | V8))) {
          num_edge_valid++;
          if (is_corner_valid(polyomino)) {
            num_corner_valid++;
          }
        }
      }
    }
    printf("%zu polyominoes of size %zu (%zu without holes)\n", polyomino_counts[size - 3], size, num_whole);
    assert(polyomino_counts[size - 3] == EXPECTED_COUNTS[size - 3]);

    printf("%zu valid eyespaces for a center group\n", num_center_valid);
    printf("%zu valid eyespaces for an edge group\n", num_edge_valid);
    printf("%zu valid eyespaces for a corner group\n", num_corner_valid);
    printf("\n");
  }

  return EXIT_SUCCESS;
}
