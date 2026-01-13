#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "tinytsumego2/scoring.h"

#include "tsumego.c"

// #define DEBUG
// #define PRINT_EXPANSION

// #define ATTACKER_PLAYS_HIGH
// #define NO_CAPTURE_DELAY

#define MAX_DEMONSTRATION (100)

#define BLOOM_SIZE (2097152)
#define BLOOM_MASK (2097151)
#define BLOOM_SHIFT (24)

#define MAX_TAIL_SIZE (8192)

// Add an entry to the bloom filter
void bloom_insert(unsigned char *bloom, stones_t a, stones_t b) {
  bloom[(a >> 3) & BLOOM_MASK] |= 1 << (a & 7);
  a >>= BLOOM_SHIFT;
  bloom[(a >> 3) & BLOOM_MASK] |= 1 << (a & 7);
  a >>= BLOOM_SHIFT;
  bloom[a >> 3] |= 1 << (a & 7);

  bloom[(b >> 3) & BLOOM_MASK] |= 1 << (b & 7);
  b >>= BLOOM_SHIFT;
  bloom[(b >> 3) & BLOOM_MASK] |= 1 << (b & 7);
  b >>= BLOOM_SHIFT;
  bloom[b >> 3] |= 1 << (b & 7);
}

// Test bloom filter membership `true` indicates likely membership in the set. `false` indicates that the element definitely isn't in the set.
bool bloom_test(unsigned char *bloom, stones_t a, stones_t b) {
  if (!(bloom[(a >> 3) & BLOOM_MASK] & (1 << (a & 7)))) {
    return false;
  }
  a >>= BLOOM_SHIFT;
  if (!(bloom[(a >> 3) & BLOOM_MASK] & (1 << (a & 7)))) {
    return false;
  }
  a >>= BLOOM_SHIFT;
  if (!(bloom[a >> 3] & (1 << (a & 7)))) {
    return false;
  }

  if (!(bloom[(b >> 3) & BLOOM_MASK] & (1 << (b & 7)))) {
    return false;
  }
  b >>= BLOOM_SHIFT;
  if (!(bloom[(b >> 3) & BLOOM_MASK] & (1 << (b & 7)))) {
    return false;
  }
  b >>= BLOOM_SHIFT;
  return bloom[b >> 3] & (1 << (b & 7));
}

// Score range for a given game state. States with loops may not converge to a single score.
typedef struct value {
  float low;
  float high;
} value;

int solve(tsumego t, bool verbose) {
  state root = t.state;

  unsigned char *bloom = calloc(BLOOM_SIZE, sizeof(unsigned char));

  size_t num_states = 1;
  size_t states_capacity = 1;
  size_t num_sorted = 0;
  state* states = malloc(states_capacity * sizeof(state));
  states[0] = root;

  size_t queue_length = 1;
  size_t queue_capacity = 1;
  state* expansion_queue = malloc(queue_capacity * sizeof(state));
  expansion_queue[0] = root;

  if (verbose) {
    print_state(&root);
  }

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

  size_t num_maybe_positive = 0;
  size_t num_false_positive = 0;
  size_t num_true_negative = 0;

  size_t num_expanded = 0;

  #ifdef DEBUG
    bool first_time_resorting = true;
  #endif

  while (num_expanded < num_states) {
    state parent = expansion_queue[--queue_length];
    #ifdef PRINT_EXPANSION
      printf("now expanding\n");
      print_state(parent);
    #endif
    for (int j = 0; j < num_moves; ++j) {
      state child = parent;
      const move_result r = make_move(&child, moves[j]);
      if (!(r == ILLEGAL || r == SECOND_PASS || r == TAKE_TARGET)) {
        stones_t child_hash_a = hash_a(&child);
        stones_t child_hash_b = hash_b(&child);
        // Pre-filter using bloom
        bool maybe_seen = bloom_test(bloom, child_hash_a, child_hash_b);
        if (maybe_seen) {
          num_maybe_positive++;
          // Binary search the sorted head
          void *existing = bsearch((void*) &child, (void*) states, num_sorted, sizeof(state), compare);
          if (existing) {
            continue;
          }
          // Search tail linearly
          bool novel = true;
          for (size_t i = num_sorted; i < num_states; ++i) {
            if (compare((void*) (states + i), (void*) &child) == 0) {
              novel = false;
              break;
            }
          }
          if (!novel) {
            continue;
          }
          num_false_positive++;
        } else {
          num_true_negative++;
        }
        if (num_states >= states_capacity) {
          num_sorted = states_capacity;
          qsort((void*) states, num_sorted, sizeof(state), compare);
          states_capacity <<= 1;
          states = realloc(states, states_capacity * sizeof(state));
          #ifdef DEBUG
            if (!first_time_resorting) {
              printf("\n");
              first_time_resorting = true;
            }
            printf("Capacity expanded to %zu\n", states_capacity);
          #endif
        } else if (num_states > num_sorted + MAX_TAIL_SIZE) {
          num_sorted = num_states;
          qsort((void*) states, num_sorted, sizeof(state), compare);
          #ifdef DEBUG
            if (first_time_resorting) {
              printf("Re-sorting at %zu", num_sorted);
            } else {
              printf(", %zu", num_sorted);
            }
            first_time_resorting = false;
          #endif
        }
        states[num_states++] = child;
        bloom_insert(bloom, child_hash_a, child_hash_b);

        queue_length++;
        if (queue_length > queue_capacity) {
          queue_capacity <<= 1;
          expansion_queue = realloc(expansion_queue, queue_capacity * sizeof(state));
          #ifdef DEBUG
            if (!first_time_resorting) {
              printf("\n");
              first_time_resorting = true;
            }
            printf("Queue capacity expanded to %zu\n", queue_capacity);
          #endif
        }
        expansion_queue[queue_length - 1] = child;
        #ifdef PRINT_EXPANSION
          printf("child\n");
          print_state(&child);
        #endif
      }
    }
    num_expanded++;
  }

  size_t bloom_bits = 0;
  for (size_t i = 0; i < BLOOM_SIZE; ++i) {
    bloom_bits += __builtin_popcount(bloom[i]);
  }

  free(bloom);
  free(expansion_queue);

  if (verbose) {
    printf("Bloom stats:\n");
    printf(" Occupancy: %g %%\n", ((double)bloom_bits) / BLOOM_SIZE / 8 * 100);
    printf(" False positive rate: %g %%\n", num_false_positive / ((double) num_maybe_positive) * 100);
    printf(" True negative rate: %g %%\n", num_true_negative / ((double) (num_maybe_positive + num_true_negative)) * 100);

    printf("Solution space size = %zu\n", num_states);
  }

  states_capacity = num_states;
  states = realloc(states, states_capacity * sizeof(state));

  qsort((void*) states, num_states, sizeof(state), compare);

  value *values = malloc(num_states * sizeof(value));

  // Initialize to unkown ranges
  for (size_t i = 0; i < num_states; ++i) {
    values[i] = (value){-INFINITY, INFINITY};
  }

  state *offset;
  bool did_update = true;
  while (did_update) {
    did_update = false;
    for (size_t i = 0; i < num_states; ++i) {
      // Don't evaluate if the range cannot be tightened
      if (values[i].low == values[i].high) {
        continue;
      }
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
          float child_score = target_lost_score(&child);
          low = fmax(low, -child_score);
          high = fmax(high, -child_score);
        } else if (r != ILLEGAL) {
          offset = (state*) bsearch((void*) &child, (void*) states, num_states, sizeof(state), compare);
          const value child_value = values[offset - states];
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

  offset = (state*) bsearch((void*) &root, (void*) states, num_states, sizeof(state), compare);
  float low = values[offset - states].low;
  float high = values[offset - states].high;
  float root_low = low;
  float root_high = high;

  if (!verbose) {
    goto cleanup;
  }

  printf("Low = %f, high = %f\n", low, high);

  // Make passing the first option when demonstrating
  for (int i = num_moves - 1; i > 0; --i) {
    moves[i] = moves[i - 1];
  }
  moves[0] = pass();

  bool low_to_play = true;
  #ifdef ATTACKER_PLAYS_HIGH
    low_to_play = false;
  #endif

  state s = root;
  for (int n = 0; n < MAX_DEMONSTRATION; ++n) {
    for (int j = 0; j < num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, moves[j]);
      if (r == TAKE_TARGET) {
        printf("%c%c: takes target (%f)\n",  column_of(moves[j]), row_of(moves[j]), target_lost_score(&child));
      } else if (r == SECOND_PASS) {
        printf("%c%c: game over (%f)\n",  column_of(moves[j]), row_of(moves[j]), score(&child));
      } else if (r != ILLEGAL) {
        offset = (state*) bsearch((void*) &child, (void*) states, num_states, sizeof(state), compare);
        const value child_value = values[offset - states];
        printf("%c%c: %f, %f\n",  column_of(moves[j]), row_of(moves[j]), child_value.low, child_value.high);
      }
    }

    for (int j = 0; j < num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, moves[j]);
      if (r == TAKE_TARGET) {
        print_state(&child);
        printf("Target captured\n");
        goto cleanup;
      } else if (r == SECOND_PASS) {
        float good_score = low_to_play ? low : high;
        if (-score(&child) == good_score) {
          print_state(&child);
          printf("Game over\n");
          goto cleanup;
        } else {
          continue;
        }
      } else if (r != ILLEGAL) {
        offset = (state*) bsearch((void*) &child, (void*) states, num_states, sizeof(state), compare);
        const value child_value = values[offset - states];

        bool good;
        if (low_to_play) {
          good = (-delay_capture(child_value.high) == low);
        } else {
          good = (-delay_capture(child_value.low) == high);
        }

        if (good) {
          s = child;
          low_to_play = !low_to_play;
          low = child_value.low;
          high = child_value.high;
          if (r == KO_THREAT_AND_RETAKE) {
            printf("Ko threat made and answered...\n");
          }
          printf("\n");
          print_state(&s);
          break;
        }
      }
    }
  }

  cleanup:
  free(values);
  free(states);

  if (root_low != t.low_delay || root_high != t.high_delay) {
    fprintf(stderr, "%f, %f\n", root_low, root_high);
  }

  assert(root_low == t.low_delay);
  assert(root_high == t.high_delay);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      solve(get_tsumego(TSUMEGO_NAMES[i]), false);
    }
    return EXIT_SUCCESS;
  } else {
    return solve(get_tsumego(argv[1]), true);
  }
}
