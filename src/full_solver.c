#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/full_solver.h"
#include "tinytsumego2/scoring.h"

size_t ceil_divz(size_t x, size_t y) {
  return (x + y - 1) / y;
}

void print_full_graph(full_graph *fg) {
  for (size_t i = 0; i < fg->num_nodes; ++i) {
    value v = fg->values[i];
    printf(
      "#%zu: %f, %f\n",
      i,
      v.low,
      v.high
    );
  }
}

full_graph create_full_graph(const state *root, bool use_delay, bool use_struggle) {
  if (use_delay && use_struggle) {
    fprintf(stderr, "Struggle not compatible with delay tactics\n");
    exit(EXIT_FAILURE);
  }

  full_graph fg = {0};

  fg.root = *root;
  fg.use_delay = use_delay;
  fg.use_struggle = use_struggle;

  fg.seen = calloc(ceil_divz(keyspace_size(root), CHAR_BIT), sizeof(unsigned char));

  fg.nodes_capacity = MIN_CAPACITY;
  fg.states = malloc(fg.nodes_capacity * sizeof(state));

  fg.queue_capacity = MIN_CAPACITY;
  fg.queue = malloc(fg.queue_capacity * sizeof(state));

  fg.num_moves = popcount(root->logical_area) + 1;
  fg.moves = malloc(fg.num_moves * sizeof(stones_t));

  int j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root->logical_area & p) {
      fg.moves[j++] = p;
    }
  }
  fg.moves[j] = pass();

  add_full_graph_state(&fg, root);

  return fg;
}

void add_full_graph_state(full_graph *fg, const state *s) {
  const size_t key = to_key(&(fg->root), s);
  const unsigned char bit = 1 << (key & 7);
  const size_t index = key >> 3;
  if (fg->seen[index] & bit) {
    return;
  }
  fg->seen[index] |= bit;

  if (fg->num_nodes >= fg->nodes_capacity) {
    fg->nodes_capacity <<= 1;
    fg->states = realloc(fg->states, fg->nodes_capacity * sizeof(state));
  }
  fg->states[fg->num_nodes++] = *s;

  fg->queue_length++;
  if (fg->queue_length > fg->queue_capacity) {
    fg->queue_capacity <<= 1;
    fg->queue = realloc(fg->queue, fg->queue_capacity * sizeof(state));
  }
  fg->queue[fg->queue_length - 1] = *s;
}

void expand_full_graph(full_graph *fg) {
  while (fg->queue_length) {
    state parent = fg->queue[--fg->queue_length];
    for (int i = 0; i < fg->num_moves; ++i) {
      state child = parent;
      move_result r = make_move(&child, fg->moves[i]);
      if (fg->use_struggle && r > TAKE_TARGET) {
        move_result status = normalize_immortal_regions(&(fg->root), &child);
        if (status <= TAKE_TARGET) {
          r = status;
        } else {
          status = struggle(&child);
          if (status <= TAKE_TARGET) {
            r = status;
          }
        }
      }
      if (r > TAKE_TARGET) {
        if (child.button < 0) {
          // States with the button flipped only differ by a fixed amount
          child.button = -child.button;
        }
       add_full_graph_state(fg, &child);
      }
    }
  }

  free(fg->seen);
  fg->seen = NULL;

  free(fg->queue);
  fg->queue_capacity = 0;
  fg->queue_length = 0;
  fg->queue = NULL;

  fg->nodes_capacity = fg->num_nodes;
  fg->states = realloc(fg->states, fg->nodes_capacity * sizeof(state));
  qsort((void*) fg->states, fg->num_nodes, sizeof(state), compare);
}

value get_full_graph_value(full_graph *fg, const state *s) {
  state *offset;
  float delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_BONUS;
    offset = (state*) bsearch((void*) &c, (void*) (fg->states), fg->num_nodes, sizeof(state), compare);
  } else {
    offset = (state*) bsearch((void*) s, (void*) (fg->states), fg->num_nodes, sizeof(state), compare);
  }
  if (!offset) {
    if (fg->use_struggle) {
      state c = *s;
      move_result status = normalize_immortal_regions(&(fg->root), &c);
      if (status > TAKE_TARGET) {
        status = struggle(&c);
      }
      if (status == TAKE_TARGET) {
        if (!c.button) {
          c.button = 1;
        }
        float c_score = target_lost_score(&c);
        return (value){c_score, c_score};
      }
      if (status == TARGET_LOST) {
        if (!c.button) {
          c.button = 1;
        }
        float c_score = take_target_score(&c);
        return (value){c_score, c_score};
      }
      if (status == SECOND_PASS) {
        float c_score = score(&c);
        return (value){c_score, c_score};
      }
    }
    return (value){NAN, NAN};
  }
  value v = fg->values[offset - fg->states];
  return (value) {
    v.low + delta,
    v.high + delta
  };
}

void solve_full_graph(full_graph *fg, bool verbose) {
  fg->values = malloc(fg->num_nodes * sizeof(value));

  // Initialize to unknown ranges
  for (size_t i = 0; i < fg->num_nodes; ++i) {
    fg->values[i] = (value){-INFINITY, INFINITY};
  }

  size_t last_updated = 0;
  size_t num_updated = 1;
  while (num_updated) {
    num_updated = 0;
    for (size_t i = 0; i < fg->num_nodes; ++i) {
      // Don't evaluate if the range cannot be tightened
      if (fg->values[i].low == fg->values[i].high) {
        continue;
      }
      // Perform negamax
      float low = fg->values[i].low;
      float high = -INFINITY;

      // Delay tactics can cause an infinite loop.
      // This would be the smallest exception to low = -INFINITY;
      // if (fg->values[i].low > -BIG_SCORE && fg->values[i].low < 1 - BIG_SCORE)
      //   low = fg->values[i].low;

      for (int j = 0; j < fg->num_moves; ++j) {
        state child = fg->states[i];
        const move_result r = make_move(&child, fg->moves[j]);
        if (fg->use_struggle && r > TAKE_TARGET) {
          move_result status = normalize_immortal_regions(&(fg->root), &child);
          if (status > TAKE_TARGET) {
            status = struggle(&child);
          }
          if (status == TAKE_TARGET) {
            if (!child.button) {
              child.button = 1;
            }
            float child_score = target_lost_score(&child);
            low = fmax(low, -child_score);
            high = fmax(high, -child_score);
            continue;
          }
          if (status == TARGET_LOST) {
            if (!child.button) {
              child.button = -1;
            }
            float child_score = take_target_score(&child);
            low = fmax(low, -child_score);
            high = fmax(high, -child_score);
            continue;
          }
          if (status == SECOND_PASS) {
            float child_score = score(&child);
            low = fmax(low, -child_score);
            high = fmax(high, -child_score);
            continue;
          }
        }
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
          if (fg->use_delay) {
            low = fmax(low, -delay_capture(child_value.high));
            high = fmax(high, -delay_capture(child_value.low));
          } else {
            low = fmax(low, -child_value.high);
            high = fmax(high, -child_value.low);
          }
        }
      }
      if (fg->values[i].low != low || fg->values[i].high != high) {
        fg->values[i] = (value) {low, high};
        num_updated++;
      }
    }
    if (verbose) {
      value v = get_full_graph_value(fg, &(fg->root));
      if (num_updated != last_updated) {
        printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
      }
      last_updated = num_updated;
    }
  }
}

void free_full_graph(full_graph *fg) {
  free(fg->moves);
  fg->num_moves = 0;
  fg->moves = NULL;

  free(fg->seen);
  fg->seen = NULL;

  free(fg->queue);
  fg->queue_capacity = 0;
  fg->queue_length = 0;
  fg->queue = NULL;

  free(fg->states);
  fg->num_nodes = 0;
  fg->nodes_capacity = 0;
  fg->states = NULL;

  free(fg->values);
  fg->values = NULL;
}
