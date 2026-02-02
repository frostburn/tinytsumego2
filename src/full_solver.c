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

  if (root->ko || root->passes) {
    fprintf(stderr, "The root state may not have an active ko or previous passes\n");
    exit(EXIT_FAILURE);
  }

  full_graph fg = {0};

  fg.root = *root;
  fg.use_delay = use_delay;
  fg.use_struggle = use_struggle;

  fg.seen = calloc(ceil_divz(tight_keyspace_size(root), CHAR_BIT), sizeof(unsigned char));

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

  if (root->button < 0) {
    state c = *root;
    c.button = -c.button;
    add_full_graph_state(&fg, &c);
  } else {
    add_full_graph_state(&fg, root);
  }

  return fg;
}

void enqueue_full_graph_state(full_graph *fg, const state *s) {
  fg->queue_length++;
  if (fg->queue_length > fg->queue_capacity) {
    fg->queue_capacity <<= 1;
    fg->queue = realloc(fg->queue, fg->queue_capacity * sizeof(state));
  }
  fg->queue[fg->queue_length - 1] = *s;
}

void add_full_graph_state(full_graph *fg, const state *s) {
  const size_t key = to_tight_key(&(fg->root), s);
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

  enqueue_full_graph_state(fg, s);
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
        if (child.passes || child.ko) {
          enqueue_full_graph_state(fg, &child);
        } else {
          add_full_graph_state(fg, &child);
        }
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
  qsort((void*) fg->states, fg->num_nodes, sizeof(state), compare_simple);
}

value get_full_graph_value(full_graph *fg, const state *s) {
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    float low = -INFINITY;
    float high = -INFINITY;

    for (int j = 0; j < fg->num_moves; ++j) {
      state child = *s;
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
        if (fg->use_delay) {
          low = fmax(low, -delay_capture(child_value.high));
          high = fmax(high, -delay_capture(child_value.low));
        } else {
          low = fmax(low, -child_value.high);
          high = fmax(high, -child_value.low);
        }
      }
    }
    return (value){low, high};
  }

  state *offset;
  float delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_BONUS;
    offset = (state*) bsearch((void*) &c, (void*) (fg->states), fg->num_nodes, sizeof(state), compare_simple);
  } else {
    offset = (state*) bsearch((void*) s, (void*) (fg->states), fg->num_nodes, sizeof(state), compare_simple);
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

void solve_full_graph(full_graph *fg, bool root_only, bool verbose) {
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
        // Strictly speaking a second pass should be impossible due to key space tightness
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

          // Not strictly necessary, but nice to have when doing a re-evaluation of a pruned graph
          if (isnan(child_value.low)) {
            high = INFINITY;
          }

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
    if (root_only) {
      value v = get_full_graph_value(fg, &(fg->root));
      if (v.low == v.high) {
        break;
      }
    }
  }
}

#define LOW_MARK (-1)
#define HIGH_MARK (-2)

#define STRUGGLE_BOILERPLATE \
if (fg->use_struggle) {\
  move_result status = normalize_immortal_regions(&(fg->root), &child);\
  if (status > TAKE_TARGET) {\
    status = struggle(&child);\
  }\
  if (status <= TAKE_TARGET) {\
    continue;\
  }\
}\

void mark_high_path(full_graph *fg, state *s);

// Mark nodes that are essential for maintaining a low value (preserves equally good moves; technically only one is required)
void mark_low_path(full_graph *fg, state *s) {
  // Non-simple state. Mark grandchildren.
  if (s->passes > 0 || s->ko) {
    for (int j = 0; j < fg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, fg->moves[j]);
      if (r > TAKE_TARGET) {
        STRUGGLE_BOILERPLATE
        child.button = abs(child.button);
        mark_high_path(fg, &child);
      }
    }
    return;
  }

  // Simple node in the graph. Refresh to get the actual node.
  s = (state*) bsearch((void*) s, (void*) (fg->states), fg->num_nodes, sizeof(state), compare_simple);

  // Already covered this iteration. Bail out
  if (s->passes < 0) {
    return;
  }

  s->passes = LOW_MARK;

  value v = fg->values[s - fg->states];
  for (int j = 0; j < fg->num_moves; ++j) {
    state child = *s;
    const move_result r = make_move(&child, fg->moves[j]);
    if (r > TAKE_TARGET) {
      STRUGGLE_BOILERPLATE
      value child_value = get_full_graph_value(fg, &child);
      if (v.low == (fg->use_delay ? -delay_capture(child_value.high) : -child_value.high)) {
        child.button = abs(child.button);
        mark_high_path(fg, &child);
      }
    }
  }
}

// Mark nodes that are essential for maintaining a high value i.e. all child nodes
void mark_high_path(full_graph *fg, state *s) {
  // Non-simple state. Mark grandchildren.
  if (s->passes > 0 || s->ko) {
    for (int j = 0; j < fg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, fg->moves[j]);
      if (r > TAKE_TARGET) {
        STRUGGLE_BOILERPLATE
        child.button = abs(child.button);
        mark_low_path(fg, &child);
      }
    }
    return;
  }

  // Simple node in the graph. Refresh to get the actual node.
  s = (state*) bsearch((void*) s, (void*) (fg->states), fg->num_nodes, sizeof(state), compare_simple);

  // Already covered this iteration. Bail out
  if (s->passes <= HIGH_MARK) {
    return;
  }

  s->passes = HIGH_MARK;

  for (int j = 0; j < fg->num_moves; ++j) {
    state child = *s;
    const move_result r = make_move(&child, fg->moves[j]);
    if (r > TAKE_TARGET) {
      STRUGGLE_BOILERPLATE
      child.button = abs(child.button);
      mark_low_path(fg, &child);
    }
  }
}

void prune_full_graph(full_graph *fg) {
  state root = fg->root;
  root.button = abs(root.button);
  mark_low_path(fg, &root);
  mark_high_path(fg, &root);

  // Prune unmarked nodes and clear marks
  size_t j = 0;
  for (size_t i = 0; i < fg->num_nodes; ++i) {
    if (fg->states[i].passes < 0) {
      fg->states[i].passes = 0;
      fg->values[j] = fg->values[i];
      fg->states[j++] = fg->states[i];
    }
  }
  fg->num_nodes = j;
  fg->states = realloc(fg->states, fg->num_nodes * sizeof(state));
  fg->values = realloc(fg->values, fg->num_nodes * sizeof(value));
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
