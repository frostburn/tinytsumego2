#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/complete_solver.h"

#define MAX_COMPENSATION_DEPTH (6)
#define MAX_INITIALIZATION_DEPTH (1024)

void print_complete_graph(complete_graph *cg) {
  for (size_t i = 0; i < cg->keyspace.size; ++i) {
    value v = cg->values[i];
    printf(
      "#%zu: %f, %f\n",
      i,
      v.low,
      v.high
    );
  }
}

complete_graph create_complete_graph(const state *root, bool use_delay) {
  if (root->ko || root->passes) {
    fprintf(stderr, "The root state may not have an active ko or previous passes\n");
    exit(EXIT_FAILURE);
  }

  complete_graph cg = {0};

  cg.keyspace = create_tight_keyspace(root);
  cg.use_delay = use_delay;

  cg.num_moves = popcount(root->logical_area) + 1;
  cg.moves = malloc(cg.num_moves * sizeof(stones_t));

  int j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root->logical_area & p) {
      cg.moves[j++] = p;
    }
  }
  cg.moves[j] = pass();

  cg.values = malloc(cg.keyspace.size * sizeof(value));

  return cg;
}

value get_complete_graph_value_(complete_graph *cg, const state *s, int depth) {
  if (!depth) {
    return (value){-INFINITY, INFINITY};
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    float low = -INFINITY;
    float high = -INFINITY;

    for (int j = 0; j < cg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, cg->moves[j]);
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
        const value child_value = get_complete_graph_value_(cg, &child, depth - 1);
        if (cg->use_delay) {
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

  size_t key;
  float delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_BONUS;
    key = to_tight_key_fast(&(cg->keyspace), &c);
  } else {
    key = to_tight_key_fast(&(cg->keyspace), s);
  }
  value v = cg->values[key];
  return (value) {
    v.low + delta,
    v.high + delta
  };
}

value get_complete_graph_value(complete_graph *cg, const state *s) {
  return get_complete_graph_value_(cg, s, MAX_COMPENSATION_DEPTH);
}

void initialize_from(complete_graph *cg, const state *s, int depth) {
  if (!depth) {
    return;
  }
  if (!s->passes && !s->ko) {
    size_t key = to_tight_key_fast(&(cg->keyspace), s);
    if (!isnan(cg->values[key].low)) {
      return;
    }
    cg->values[key] = (value){-INFINITY, INFINITY};
  }

  for (int j = 0; j < cg->num_moves; ++j) {
    state child = *s;
    const move_result r = make_move(&child, cg->moves[j]);
    if (r > TAKE_TARGET) {
      if (child.button < 0) {
        child.button = -child.button;
      }
      initialize_from(cg, &child, depth - 1);
    }
  }
}

void solve_complete_graph(complete_graph *cg, bool root_only, bool verbose) {
  // Initialize to unknown ranges
  if (root_only) {
    for (size_t i = 0; i < cg->keyspace.size; ++i) {
      cg->values[i] = (value){NAN, NAN};
    }
    state root = cg->keyspace.root;
    if (root.button < 0) {
      root.button = -root.button;
    }
    initialize_from(cg, &root, MAX_INITIALIZATION_DEPTH);
  } else {
    for (size_t i = 0; i < cg->keyspace.size; ++i) {
      state s = from_tight_key_fast(&(cg->keyspace), i);
      if (is_legal(&s)) {
        cg->values[i] = (value){-INFINITY, INFINITY};
      } else {
        cg->values[i] = (value){NAN, NAN};
      }
    }
  }

  size_t last_updated = 0;
  size_t num_updated = 1;
  while (num_updated) {
    num_updated = 0;
    for (size_t i = 0; i < cg->keyspace.size; ++i) {
      // Skip illegal/irrelevant states
      if (isnan(cg->values[i].low)) {
        continue;
      }
      // Don't evaluate if the range cannot be tightened
      if (cg->values[i].low == cg->values[i].high) {
        continue;
      }
      // Perform negamax
      float low = cg->values[i].low;
      float high = -INFINITY;

      // Delay tactics can cause an infinite loop.
      // This would be the smallest exception to low = -INFINITY;
      // if (cg->values[i].low > -BIG_SCORE && cg->values[i].low < 1 - BIG_SCORE)
      //   low = cg->values[i].low;

      state parent = from_tight_key_fast(&(cg->keyspace), i);
      for (int j = 0; j < cg->num_moves; ++j) {
        state child = parent;
        const move_result r = make_move(&child, cg->moves[j]);
        // Second pass cannot happen here due to keyspace tightness
        if (r == TAKE_TARGET) {
          float child_score = target_lost_score(&child);
          low = fmax(low, -child_score);
          high = fmax(high, -child_score);
        } else if (r != ILLEGAL) {
          const value child_value = get_complete_graph_value(cg, &child);
          if (cg->use_delay) {
            low = fmax(low, -delay_capture(child_value.high));
            high = fmax(high, -delay_capture(child_value.low));
          } else {
            low = fmax(low, -child_value.high);
            high = fmax(high, -child_value.low);
          }
        }
      }
      if (cg->values[i].low != low || cg->values[i].high != high) {
        cg->values[i] = (value) {low, high};
        num_updated++;
      }
    }
    if (verbose) {
      value v = get_complete_graph_value(cg, &(cg->keyspace.root));
      if (num_updated != last_updated) {
        printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
      }
      last_updated = num_updated;
    }
    if (root_only) {
      value v = get_complete_graph_value(cg, &(cg->keyspace.root));
      if (v.low == v.high) {
        break;
      }
    }
  }
}

void free_complete_graph(complete_graph *cg) {
  free_tight_keyspace(&(cg->keyspace));

  free(cg->moves);
  cg->num_moves = 0;
  cg->moves = NULL;

  free(cg->values);
  cg->values = NULL;
}
