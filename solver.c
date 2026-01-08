#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "tsumego.c"

// #define PRINT_EXPANSION

// Large value for capturing the target stones
#define TARGET_SCORE (1000)

// The maximum regular score (and then some)
#define BIG_SCORE (WIDTH * HEIGHT + 10)

float score(state *s) {
  return (
    chinese_liberty_score(s) +
    s->button * 0.5 +
    s->ko_threats * 0.0625
  );
}

float delay_capture(float my_score) {
  if (my_score < -BIG_SCORE) {
    return my_score + 0.25;
  }
  return my_score;
}

// Score range for a given game state. States with loops may not converge to a single score.
typedef struct value {
  float low;
  float high;
} value;

int main() {
  state root = get_tsumego("Straight Three");

  size_t num_states = 1;
  size_t states_capacity = 1;
  state* states = malloc(states_capacity * sizeof(state));
  states[0] = root;

  print_state(&root);

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

  size_t num_expanded = 0;

  while (num_expanded < num_states) {
    for (int j = 0; j < num_moves; ++j) {
      state child = states[num_expanded];
      const move_result r = make_move(&child, moves[j]);
      if (!(r == ILLEGAL || r == SECOND_PASS || r == TAKE_TARGET)) {
        bool novel = true;
        for (size_t i = 0; i < num_states; ++i) {
          if (equals(states + i, &child)) {
            novel = false;
            break;
          }
        }
        if (!novel) {
          continue;
        }
        num_states++;
        if (num_states > states_capacity) {
          states_capacity <<= 1;
          states = realloc(states, states_capacity * sizeof(state));
        }
        states[num_states - 1] = child;
        #ifdef PRINT_EXPANSION
          print_state(&child);
        #endif
      }
    }
    num_expanded++;
  }

  printf("Solution space size = %zu\n", num_states);

  states_capacity = num_states;
  states = realloc(states, states_capacity * sizeof(state));
  value *values = malloc(num_states * sizeof(value));

  // Initialize to unkown ranges
  for (size_t i = 0; i < num_states; ++i) {
    values[i] = (value){-INFINITY, INFINITY};
  }

  bool did_update = true;
  while (did_update) {
    did_update = false;
    for (size_t i = 0; i < num_states; ++i) {
      // Perform negamax
      float low = -INFINITY;
      float high = -INFINITY;

      for (int j = 0; j < num_moves; ++j) {
        state child = states[i];
        const move_result r = make_move(&child, moves[j]);
        if (r == SECOND_PASS) {
          float child_score = score(&child);
          low = fmax(low, -child_score);
          high = fmax(high, -child_score);
        }
        else if (r == TAKE_TARGET) {
          float child_score = score(&child) - TARGET_SCORE;
          low = fmax(low, -child_score);
          high = fmax(high, -child_score);
        } else if (r != ILLEGAL) {
          // Linear search. TODO: Convert to dictionary lookup
          size_t k = 0;
          for (;k < num_states; ++k) {
            if (equals(states + k, &child)) {
              break;
            }
          }
          const value child_value = values[k];
          low = fmax(low, -delay_capture(child_value.high));
          high = fmax(high, -delay_capture(child_value.low));
        }
      }
      if (values[i].low != low || values[i].high != high) {
        values[i] = (value) {low, high};
        did_update = true;
      }
    }
  }

  state s = root;
  float low = values[0].low;
  float high = values[0].high;

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
        if (-score(&child) == low) {
          print_state(&child);
          printf("Game over\n");
          goto cleanup;
        } else {
          continue;
        }
      }
      if (r != ILLEGAL) {
        size_t k = 0;
        for (;k < num_states; ++k) {
          if (equals(states + k, &child)) {
            break;
          }
        }
        const value child_value = values[k];

        if (-delay_capture(child_value.high) == low) {
          s = child;
          low = child_value.low;
          high = child_value.high;
          if (r == KO_THREAT_AND_RETAKE) {
            printf("Ko threat made and answered...\n");
          }
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
