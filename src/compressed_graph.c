#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/compressed_graph.h"

void print_compressed_graph(compressed_graph *cg) {
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

compressed_graph create_compressed_graph(const state *root) {
  if (root->ko || root->passes) {
    fprintf(stderr, "The root state may not have an active ko or previous passes\n");
    exit(EXIT_FAILURE);
  }

  compressed_graph cg = {0};

  cg.keyspace = create_compressed_keyspace(root);

  cg.moves = moves_of(root, &cg.num_moves);

  cg.values = malloc(cg.keyspace.size * sizeof(table_value));

  for (size_t i = 0; i < cg.keyspace.size; ++i) {
    cg.values[i] = MAX_RANGE_Q7;
  }

  return cg;
}

compressed_graph* allocate_compressed_graph(const state *root) {
  compressed_graph *result = malloc(sizeof(compressed_graph));
  *result = create_compressed_graph(root);
  return result;
}

table_value get_compressed_graph_value_(compressed_graph *cg, const state *s, int depth) {
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
      table_value child_value;
      if (r <= TAKE_TARGET) {
        child_value = score_terminal_q7(r, &child);
      } else {
        child_value = apply_tactics_q7(NONE, r, &child, get_compressed_graph_value_(cg, &child, depth - 1));
      }
      if (child_value.high > low) low = child_value.high;
      if (child_value.low > high) high = child_value.low;
    }
    return (table_value) {low, high};
  }

  size_t key;
  score_q7_t delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_Q7;
    key = to_compressed_key(&(cg->keyspace), &c);
  } else {
    key = to_compressed_key(&(cg->keyspace), s);
  }
  table_value result = cg->values[key];
  if (result.low != SCORE_Q7_MIN) {
    result.low += delta;
  }
  if (result.high != SCORE_Q7_MAX) {
    result.high += delta;
  }
  return result;
}

value get_compressed_graph_value(compressed_graph *cg, const state *s) {
  return table_value_to_value(get_compressed_graph_value_(cg, s, MAX_COMPENSATION_DEPTH));
}

bool iterate_compressed_graph(compressed_graph *cg, bool verbose) {
  size_t num_updated = 0;
  for (size_t k = 0; k < cg->keyspace.keyspace.size; ++k) {
    if (!was_legal(&(cg->keyspace), k)) {
      continue;
    }
    size_t i = remap_tight_key(&(cg->keyspace), k);
    // Don't evaluate if the range cannot be tightened
    if (cg->values[i].low == cg->values[i].high) {
      continue;
    }
    // Perform negamax (with memory to break delay shuffling)
    score_q7_t low = cg->values[i].low;
    score_q7_t high = SCORE_Q7_MIN;

    state parent = from_tight_key_fast(&(cg->keyspace.keyspace), k);
    for (int j = 0; j < cg->num_moves; ++j) {
      state child = parent;
      const move_result r = make_move(&child, cg->moves[j]);
      table_value child_value;
      if (r <= TAKE_TARGET) {
        child_value = score_terminal_q7(r, &child);
      } else {
        child_value = get_compressed_graph_value_(cg, &child, MAX_COMPENSATION_DEPTH);
        child_value = apply_tactics_q7(NONE, r, &child, child_value);
      }
      if (child_value.high > low) low = child_value.high;
      if (child_value.low > high) high = child_value.low;
    }
    if (cg->values[i].low != low || cg->values[i].high != high) {
      cg->values[i].low = low;
      cg->values[i].high = high;
      num_updated++;
    }
  }
  if (verbose) {
    value v = get_compressed_graph_value(cg, &(cg->keyspace.keyspace.root));
    printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
  }
  return num_updated;
}

void free_compressed_graph(compressed_graph *cg) {
  free_compressed_keyspace(&(cg->keyspace));

  free(cg->moves);
  cg->num_moves = 0;
  cg->moves = NULL;

  free(cg->values);
  cg->values = NULL;
}
