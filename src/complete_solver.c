#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/complete_solver.h"

void print_complete_graph(complete_graph *cg) {
  size_t num_nodes = tight_keyspace_size(&(cg->root));
  for (size_t i = 0; i < num_nodes; ++i) {
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

  cg.root = *root;
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

  cg.values = malloc(tight_keyspace_size(root) * sizeof(value));

  return cg;
}

value get_complete_graph_value(complete_graph *cg, const state *s) {
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
    return (value){low, high};
  }

  size_t key;
  float delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_BONUS;
    key = to_tight_key(&(cg->root), &c);
  } else {
    key = to_tight_key(&(cg->root), s);
  }
  value v = cg->values[key];
  return (value) {
    v.low + delta,
    v.high + delta
  };
}

void solve_complete_graph(complete_graph *cg, bool root_only, bool verbose) {
  size_t num_nodes = tight_keyspace_size(&(cg->root));

  // Initialize to unknown ranges
  for (size_t i = 0; i < num_nodes; ++i) {
    cg->values[i] = (value){-INFINITY, INFINITY};
  }

  size_t last_updated = 0;
  size_t num_updated = 1;
  while (num_updated) {
    num_updated = 0;
    for (size_t i = 0; i < num_nodes; ++i) {
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

      state parent = from_tight_key(&(cg->root), i);
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
      value v = get_complete_graph_value(cg, &(cg->root));
      if (num_updated != last_updated) {
        printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
      }
      last_updated = num_updated;
    }
    if (root_only) {
      value v = get_complete_graph_value(cg, &(cg->root));
      if (v.low == v.high) {
        break;
      }
    }
  }
}

void free_complete_graph(complete_graph *cg) {
  free(cg->moves);
  cg->num_moves = 0;
  cg->moves = NULL;

  free(cg->values);
  cg->values = NULL;
}
