#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/complete_solver.h"

#define MAX_COMPENSATION_DEPTH (6)
#define MAX_INITIALIZATION_DEPTH (1024)

void print_complete_graph(complete_graph *cg) {
  for (size_t i = 0; i < cg->keyspace.size; ++i) {
    value v = table_value_to_value(cg->values[i]);
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

  cg.values = malloc(cg.keyspace.size * sizeof(table_value));

  return cg;
}

table_value get_complete_graph_value_(complete_graph *cg, const state *s, int depth) {
  if (!depth) {
    return MAX_RANGE_Q7;
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    score_q7_t low = SCORE_Q7_MIN;
    score_q7_t high = SCORE_Q7_MIN;

    for (int j = 0; j < cg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, cg->moves[j]);
      if (r == SECOND_PASS) {
        score_q7_t child_score = -score_q7(&child);
        if (child_score > low) low = child_score;
        if (child_score > high) high = child_score;
      }
      else if (r == TAKE_TARGET) {
        score_q7_t child_score = -target_lost_score_q7(&child);
        if (child_score > low) low = child_score;
        if (child_score > high) high = child_score;
      } else if (r != ILLEGAL) {
        table_value child_value = get_complete_graph_value_(cg, &child, depth - 1);
        if (cg->use_delay) {
          child_value.low = -delay_capture_q7(child_value.low);
          child_value.high = -delay_capture_q7(child_value.high);
        } else {
          child_value.low = -child_value.low;
          child_value.high = -child_value.high;
        }
        if (child_value.high > low) low = child_value.high;
        if (child_value.low > high) high = child_value.low;
      }
    }
    return (table_value){low, high};
  }

  size_t key;
  score_q7_t delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_Q7;
    key = to_tight_key_fast(&(cg->keyspace), &c);
  } else {
    key = to_tight_key_fast(&(cg->keyspace), s);
  }
  table_value v = cg->values[key];
  if (v.low == SCORE_Q7_NAN) {
    return NAN_RANGE_Q7;
  }
  if (v.low != SCORE_Q7_MIN) {
    v.low += delta;
  }
  if (v.high != SCORE_Q7_MAX) {
    v.high += delta;
  }
  return v;
}

value get_complete_graph_value(complete_graph *cg, const state *s) {
  return table_value_to_value(get_complete_graph_value_(cg, s, MAX_COMPENSATION_DEPTH));
}

void initialize_from(complete_graph *cg, const state *s, int depth) {
  if (!depth) {
    return;
  }
  if (!s->passes && !s->ko) {
    size_t key = to_tight_key_fast(&(cg->keyspace), s);
    if (cg->values[key].low != SCORE_Q7_NAN) {
      return;
    }
    cg->values[key] = MAX_RANGE_Q7;
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
      cg->values[i] = NAN_RANGE_Q7;
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
        cg->values[i] = MAX_RANGE_Q7;
      } else {
        cg->values[i] = NAN_RANGE_Q7;
      }
    }
  }

  size_t last_updated = 0;
  size_t num_updated = 1;
  while (num_updated) {
    num_updated = 0;
    for (size_t i = 0; i < cg->keyspace.size; ++i) {
      // Skip illegal/irrelevant states
      if (cg->values[i].low == SCORE_Q7_NAN) {
        continue;
      }
      // Don't evaluate if the range cannot be tightened
      if (cg->values[i].low == cg->values[i].high) {
        continue;
      }
      // Perform negamax (with memory to break delay shuffling)
      score_q7_t low = cg->values[i].low;
      score_q7_t high = SCORE_Q7_MIN;

      state parent = from_tight_key_fast(&(cg->keyspace), i);
      for (int j = 0; j < cg->num_moves; ++j) {
        state child = parent;
        const move_result r = make_move(&child, cg->moves[j]);
        // Second pass cannot happen here due to keyspace tightness
        if (r == TAKE_TARGET) {
          score_q7_t child_score = -target_lost_score_q7(&child);
          if (child_score > low) low = child_score;
          if (child_score > high) high = child_score;
        } else if (r != ILLEGAL) {
          table_value child_value = get_complete_graph_value_(cg, &child, MAX_COMPENSATION_DEPTH);
          if (cg->use_delay) {
            child_value.low = -delay_capture_q7(child_value.low);
            child_value.high = -delay_capture_q7(child_value.high);
          } else {
            child_value.low = -child_value.low;
            child_value.high = -child_value.high;
          }
          if (child_value.high > low) low = child_value.high;
          if (child_value.low > high) high = child_value.low;
        }
      }
      if (cg->values[i].low != low || cg->values[i].high != high) {
        cg->values[i] = (table_value) {low, high};
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
