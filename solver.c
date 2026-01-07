#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "tsumego.c"

// Large value for capturing the target stones
#define TARGET_SCORE (10000)

// #define PRINT_EXPANSION

// Score range for a given game state. States with loops may not converge to a single score.
typedef struct value {
  float low;
  float high;
} value;

int main() {
  state root = get_tsumego("Straight Three");
  const size_t size = keyspace_size(&root);

  value* values = malloc(size * sizeof(value));
  state* states = malloc(size * sizeof(state));

  // Initialize solution space to unknowns
  for (size_t i = 0; i < size; ++i) {
    values[i] = (value) {NAN, NAN};
    states[i].logical_area = 0;
  }

  print_state(&root);
  printf("Key space size = %zu\n", size);

  int num_moves = popcount(root.logical_area) + 1;
  stones_t* moves = malloc(num_moves * sizeof(stones_t));

  int j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root.logical_area & p) {
      moves[j++] = p;
    }
  }
  moves[j] = pass();

  const size_t root_key = to_key(&root, &root);
  values[root_key] = (value) {0, 0}; // temp value
  states[root_key] = root;

  size_t count = 1;
  bool did_expand = true;

  while (did_expand) {
    did_expand = false;

    for (size_t i = 0; i < size; ++i) {
      if (values[i].low != 0 && values[i].high != 0) {
        continue;
      }
      values[i] = (value) {-INFINITY, INFINITY};
      for (int j = 0; j < num_moves; ++j) {
        state child = states[i];
        const move_result r = make_move(&child, moves[j]);
        if (!(r == ILLEGAL || r == SECOND_PASS || r == TAKE_TARGET)) {
          const size_t child_key = to_key(&root, &child);

          // Only overwrite empty entries
          if (states[child_key].logical_area != 0) {
            continue;
          }

          values[child_key] = (value) {0, 0}; // Mark for expansion
          states[child_key] = child;
          count++;
          did_expand = true;
          #ifdef PRINT_EXPANSION
            print_state(&child);
          #endif
        }
      }
    }
  }

  printf("Solution space size = %zu\n", count);

  bool did_update = true;
  while (did_update) {
    did_update = false;
    for (size_t i = 0; i < size; ++i) {
      if (states[i].logical_area == 0) {
        continue;
      }

      // Perform negamax
      float low = -INFINITY;
      float high = -INFINITY;

      for (int j = 0; j < num_moves; ++j) {
        state child = states[i];
        const move_result r = make_move(&child, moves[j]);
        if (r == SECOND_PASS) {
          // TODO: Implement Chinese-like scoring
          low = fmax(low, 0);
          high = fmax(high, 0);
        }
        else if (r == TAKE_TARGET) {
          low = fmax(low, TARGET_SCORE);
          high = fmax(high, TARGET_SCORE);
        } else if (r != ILLEGAL) {
          // TODO: Implement delay tactics if loss of target stones is inevitable
          const size_t child_key = to_key(&root, &child);
          const value child_value = values[child_key];
          low = fmax(low, -child_value.high);
          high = fmax(high, -child_value.low);
        }
      }
      if (values[i].low != low || values[i].high != high) {
        values[i] = (value) {low, high};
        did_update = true;
      }
    }
  }

  state s = root;
  float low = values[root_key].low;
  float high = values[root_key].high;

  printf("Low = %f, high = %f\n", low, high);

  while (true) {
    for (int j = 0; j < num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, moves[j]);
      if (r == TAKE_TARGET) {
        print_state(&child);
        printf("Target captured\n");
        goto cleanup;
      }
      if (r == SECOND_PASS) {
        print_state(&child);
        printf("Game over\n");
        goto cleanup;
      }
      if (r != ILLEGAL) {
        const size_t child_key = to_key(&root, &child);
        const value child_value = values[child_key];

        if (-child_value.high == low) {
          s = child;
          low = child_value.low;
          high = child_value.high;
          print_state(&s);
          break;
        }
      }
    }
  }

  cleanup:
  free(values);
  free(states);

  return EXIT_SUCCESS;
}
