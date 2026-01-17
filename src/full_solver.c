#include <math.h>
#include "tinytsumego2/full_solver.h"
#include "tinytsumego2/scoring.h"

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

full_graph create_full_graph(state root) {
  full_graph fg = {0};

  fg.bloom = calloc(BLOOM_SIZE, sizeof(unsigned char));

  fg.nodes_capacity = MIN_CAPACITY;
  fg.num_nodes = 1;
  fg.num_sorted = 1;
  fg.states = malloc(fg.nodes_capacity * sizeof(state));
  fg.states[0] = root;

  fg.queue_length = 1;
  fg.queue_capacity = MIN_CAPACITY;
  fg.queue = malloc(fg.queue_capacity * sizeof(state));
  fg.queue[0] = root;

  fg.num_moves = popcount(root.logical_area) + 1;
  fg.moves = malloc(fg.num_moves * sizeof(stones_t));

  int j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root.logical_area & p) {
      fg.moves[j++] = p;
    }
  }
  fg.moves[j] = pass();

  // Debug variables
  size_t num_maybe_positive = 0;
  size_t num_false_positive = 0;
  size_t num_true_negative = 0;

  size_t num_expanded = 0;

  while (num_expanded < fg.num_nodes) {
    state parent = fg.queue[--fg.queue_length];
    for (int i = 0; i < fg.num_moves; ++i) {
      state child = parent;
      const move_result r = make_move(&child, fg.moves[i]);
      if (r > TAKE_TARGET) {
        const stones_t child_hash_a = hash_a(&child);
        const stones_t child_hash_b = hash_b(&child);
        // Pre-filter using bloom
        const bool maybe_seen = bloom_test(fg.bloom, child_hash_a, child_hash_b);
        if (maybe_seen) {
          num_maybe_positive++;
          // Binary search the sorted head
          void *existing = bsearch((void*) &child, (void*) fg.states, fg.num_sorted, sizeof(state), compare);
          if (existing) {
            continue;
          }
          // Search tail linearly
          bool novel = true;
          for (size_t j = fg.num_sorted; j < fg.num_nodes; ++j) {
            if (compare((void*) (fg.states + j), (void*) &child) == 0) {
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
        if (fg.num_nodes >= fg.nodes_capacity) {
          fg.num_sorted = fg.nodes_capacity;
          qsort((void*) fg.states, fg.num_sorted, sizeof(state), compare);
          fg.nodes_capacity <<= 1;
          fg.states = realloc(fg.states, fg.nodes_capacity * sizeof(state));
        } else if (fg.num_nodes > fg.num_sorted + MAX_TAIL_SIZE) {
          fg.num_sorted = fg.num_nodes;
          qsort((void*) fg.states, fg.num_sorted, sizeof(state), compare);
        }
        fg.states[fg.num_nodes++] = child;
        bloom_insert(fg.bloom, child_hash_a, child_hash_b);

        fg.queue_length++;
        if (fg.queue_length > fg.queue_capacity) {
          fg.queue_capacity <<= 1;
          fg.queue = realloc(fg.queue, fg.queue_capacity * sizeof(state));
        }
        fg.queue[fg.queue_length - 1] = child;
      }
    }
    num_expanded++;
  }

  free(fg.bloom);
  fg.bloom = NULL;

  free(fg.queue);
  fg.queue_capacity = 0;
  fg.queue_length = 0;
  fg.queue = NULL;

  fg.nodes_capacity = fg.num_nodes;
  fg.states = realloc(fg.states, fg.nodes_capacity * sizeof(state));
  fg.num_sorted = fg.num_nodes;
  qsort((void*) fg.states, fg.num_sorted, sizeof(state), compare);

  return fg;
}

value get_full_graph_value(full_graph *fg, state *s) {
  state *offset = (state*) bsearch((void*) s, (void*) (fg->states), fg->num_sorted, sizeof(state), compare);
  return fg->values[offset - fg->states];
}

void solve_full_graph(full_graph *fg) {
  fg->values = malloc(fg->num_nodes * sizeof(value));

  // Initialize to unknown ranges
  for (size_t i = 0; i < fg->num_nodes; ++i) {
    fg->values[i] = (value){-INFINITY, INFINITY};
  }

  bool did_update = true;
  while (did_update) {
    did_update = false;
    for (size_t i = 0; i < fg->num_nodes; ++i) {
      // Don't evaluate if the range cannot be tightened
      if (fg->values[i].low == fg->values[i].high) {
        continue;
      }
      // Perform negamax
      float low = -INFINITY;
      float high = -INFINITY;

      for (int j = 0; j < fg->num_moves; ++j) {
        state child = fg->states[i];
        const move_result r = make_move(&child, fg->moves[j]);
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
          const value child_value = get_full_graph_value(fg, &child);
          low = fmax(low, -delay_capture(child_value.high));
          high = fmax(high, -delay_capture(child_value.low));
        }
      }
      if (fg->values[i].low != low || fg->values[i].high != high) {
        fg->values[i] = (value) {low, high};
        did_update = true;
      }
    }
  }
}

void free_full_graph(full_graph *fg) {
  free(fg->moves);
  fg->num_moves = 0;
  fg->moves = NULL;

  free(fg->bloom);
  fg->bloom = NULL;

  free(fg->queue);
  fg->queue_capacity = 0;
  fg->queue_length = 0;
  fg->queue = NULL;

  free(fg->states);
  fg->num_nodes = 0;
  fg->nodes_capacity = 0;
  fg->num_sorted = 0;
  fg->states = NULL;

  free(fg->values);
  fg->values = NULL;
}
