#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/dual_solver.h"

void print_dual_graph(dual_graph *dg) {
  for (size_t i = 0; i < dg->keyspace.size; ++i) {
    value pv = table_value_to_value(dg->plain_values[i]);
    value fv = table_value_to_value(dg->forcing_values[i]);
    printf(
      "#%zu: (%f, %f) / (%f, %f)\n",
      i,
      pv.low,
      pv.high,
      fv.low,
      fv.high
    );
  }
}

dual_graph create_dual_graph(const state *root) {
  if (root->ko || root->passes) {
    fprintf(stderr, "The root state may not have an active ko or previous passes\n");
    exit(EXIT_FAILURE);
  }

  dual_graph dg = {0};

  dg.keyspace = create_compressed_keyspace(root);

  dg.num_moves = popcount(root->logical_area) + 1;
  dg.moves = malloc(dg.num_moves * sizeof(stones_t));

  int j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root->logical_area & p) {
      dg.moves[j++] = p;
    }
  }
  dg.moves[j] = pass();

  dg.plain_values = malloc(dg.keyspace.size * sizeof(table_value));
  dg.forcing_values = malloc(dg.keyspace.size * sizeof(table_value));

  for (size_t i = 0; i < dg.keyspace.size; ++i) {
    dg.plain_values[i] = MAX_RANGE_Q7;
    dg.forcing_values[i] = MAX_RANGE_Q7;
  }

  return dg;
}

void get_dual_graph_values(dual_graph *dg, const state *s, int depth, table_value *plain_value, table_value *forcing_value) {
  if (!depth) {
    *plain_value = MAX_RANGE_Q7;
    *forcing_value = MAX_RANGE_Q7;
    return;
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    plain_value->low = SCORE_Q7_MIN;
    plain_value->high = SCORE_Q7_MIN;
    forcing_value->low = SCORE_Q7_MIN;
    forcing_value->high = SCORE_Q7_MIN;

    for (int j = 0; j < dg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, dg->moves[j]);
      table_value child_plain;
      table_value child_forcing;
      if (r <= TAKE_TARGET) {
        child_plain = score_terminal_q7(r, &child);
        child_forcing = child_plain;
      } else {
        get_dual_graph_values(dg, &child, depth - 1, &child_plain, &child_forcing);
        child_plain = apply_tactics_q7(NONE, r, &child, child_plain);
        child_forcing = apply_tactics_q7(FORCING, r, &child, child_forcing);
      }
      if (child_plain.high > plain_value->low) plain_value->low = child_plain.high;
      if (child_plain.low > plain_value->high) plain_value->high = child_plain.low;
      if (child_forcing.high > forcing_value->low) forcing_value->low = child_forcing.high;
      if (child_forcing.low > forcing_value->high) forcing_value->high = child_forcing.low;
    }
    return;
  }

  size_t key;
  score_q7_t delta = 0;
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    delta = -2 * BUTTON_Q7;
    key = to_compressed_key(&(dg->keyspace), &c);
  } else {
    key = to_compressed_key(&(dg->keyspace), s);
  }
  *plain_value = dg->plain_values[key];
  *forcing_value = dg->forcing_values[key];
  if (plain_value->low != SCORE_Q7_MIN) {
    plain_value->low += delta;
  }
  if (plain_value->high != SCORE_Q7_MAX) {
    plain_value->high += delta;
  }
  if (forcing_value->low != SCORE_Q7_MIN) {
    forcing_value->low += delta;
  }
  if (forcing_value->high != SCORE_Q7_MAX) {
    forcing_value->high += delta;
  }
}

value get_dual_graph_value(dual_graph *dg, const state *s, tactics ts) {
  table_value plain_value;
  table_value forcing_value;
  get_dual_graph_values(dg, s, MAX_COMPENSATION_DEPTH, &plain_value, &forcing_value);
  switch (ts) {
    case NONE:
      return table_value_to_value(plain_value);
    case FORCING:
      return table_value_to_value(forcing_value);
    case DELAY:
      break;
  }
  fprintf(stderr, "Unsupported tactic");
  exit(EXIT_FAILURE);
  return (value) {NAN, NAN};
}

bool iterate_dual_graph(dual_graph *dg, bool verbose) {
  size_t num_updated = 0;
  for (size_t k = 0; k < dg->keyspace.keyspace.size; ++k) {
    if (!was_legal(&(dg->keyspace), k)) {
      continue;
    }
    size_t i = remap_tight_key(&(dg->keyspace), k);
    // Don't evaluate if the range cannot be tightened
    if (dg->plain_values[i].low == dg->plain_values[i].high && dg->forcing_values[i].low == dg->forcing_values[i].high) {
      continue;
    }
    // Perform negamax (with memory to break delay shuffling)
    score_q7_t plain_low = dg->plain_values[i].low;
    score_q7_t plain_high = SCORE_Q7_MIN;
    score_q7_t forcing_low = dg->forcing_values[i].low;
    score_q7_t forcing_high = SCORE_Q7_MIN;

    state parent = from_tight_key_fast(&(dg->keyspace.keyspace), k);
    for (int j = 0; j < dg->num_moves; ++j) {
      state child = parent;
      const move_result r = make_move(&child, dg->moves[j]);
      table_value child_plain;
      table_value child_forcing;
      if (r <= TAKE_TARGET) {
        child_plain = score_terminal_q7(r, &child);
        child_forcing = child_plain;
      } else {
        get_dual_graph_values(dg, &child, MAX_COMPENSATION_DEPTH, &child_plain, &child_forcing);
        child_plain = apply_tactics_q7(NONE, r, &child, child_plain);
        child_forcing = apply_tactics_q7(FORCING, r, &child, child_forcing);
      }
      if (child_plain.high > plain_low) plain_low = child_plain.high;
      if (child_plain.low > plain_high) plain_high = child_plain.low;
      if (child_forcing.high > forcing_low) forcing_low = child_forcing.high;
      if (child_forcing.low > forcing_high) forcing_high = child_forcing.low;
    }
    if (
      dg->plain_values[i].low != plain_low || dg->plain_values[i].high != plain_high ||
      dg->forcing_values[i].low != forcing_low || dg->forcing_values[i].high != forcing_high
    ) {
      dg->plain_values[i] = (table_value) {plain_low, plain_high};
      dg->forcing_values[i] = (table_value) {forcing_low, forcing_high};
      num_updated++;
    }
  }
  if (verbose) {
    value v = get_dual_graph_value(dg, &(dg->keyspace.keyspace.root), NONE);
    printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
  }
  return num_updated;
}

void free_dual_graph(dual_graph *dg) {
  free_compressed_keyspace(&(dg->keyspace));

  free(dg->moves);
  dg->num_moves = 0;
  dg->moves = NULL;

  free(dg->plain_values);
  dg->plain_values = NULL;
  free(dg->forcing_values);
  dg->forcing_values = NULL;
}
