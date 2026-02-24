#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "jkiss/jkiss.h"
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

  dg.moves = moves_of(root, &dg.num_moves);

  dg.plain_values = malloc(dg.keyspace.size * sizeof(table_value));
  dg.forcing_values = malloc(dg.keyspace.size * sizeof(table_value));

  for (size_t i = 0; i < dg.keyspace.size; ++i) {
    dg.plain_values[i] = MAX_RANGE_Q7;
    dg.forcing_values[i] = MAX_RANGE_Q7;
  }

  return dg;
}

dual_graph* allocate_dual_graph(const state *root) {
  dual_graph *result = malloc(sizeof(dual_graph));
  *result = create_dual_graph(root);
  return result;
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

table_value get_dual_graph_area_value_(dual_graph *dg, const state *s, int depth) {
  if (!depth) {
    return MAX_RANGE_Q7;
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    score_q7_t low = SCORE_Q7_MIN;
    score_q7_t high = SCORE_Q7_MIN;

    for (int j = 0; j < dg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, dg->moves[j]);
      table_value child_value;
      if (r == SECOND_PASS) {
        // For the most part true area scoring agrees with simple area scoring,
        // but removing ko-threats after two passes makes
        // "Bent Four in the Corner is Dead"-type cases into dead shapes instead of seki.
        score_q7_t delta = child.ko_threats * KO_THREAT_Q7;
        child.ko = 0ULL;
        child.ko_threats = 0;
        child.passes = 0;
        child_value = get_dual_graph_area_value_(dg, &child, depth - 1);
        child_value.low += delta;
        child_value.high += delta;
        child_value = apply_tactics_q7(NONE, r, &child, child_value);
      } else if (r <= TAKE_TARGET) {
        child_value = score_terminal_q7(r, &child);
      } else {
        child_value = get_dual_graph_area_value_(dg, &child, depth - 1);
        child_value = apply_tactics_q7(NONE, r, &child, child_value);
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
    key = to_compressed_key(&(dg->keyspace), &c);
  } else {
    key = to_compressed_key(&(dg->keyspace), s);
  }
  table_value v = dg->plain_values[key];
  if (v.low != SCORE_Q7_MIN) {
    v.low += delta;
  }
  if (v.high != SCORE_Q7_MAX) {
    v.high += delta;
  }
  return v;
}

bool area_iterate_dual_graph(dual_graph *dg, bool verbose) {
  size_t num_updated = 0;
  for (size_t k = 0; k < dg->keyspace.keyspace.size; ++k) {
    if (!was_legal(&(dg->keyspace), k)) {
      continue;
    }
    size_t i = remap_tight_key(&(dg->keyspace), k);

    // Perform negamax
    score_q7_t low = SCORE_Q7_MIN;
    score_q7_t high = SCORE_Q7_MIN;

    state parent = from_tight_key_fast(&(dg->keyspace.keyspace), k);
    for (int j = 0; j < dg->num_moves; ++j) {
      state child = parent;
      const move_result r = make_move(&child, dg->moves[j]);
      table_value child_value;
      if (r <= TAKE_TARGET) {
        child_value = score_terminal_q7(r, &child);
      } else {
        child_value = get_dual_graph_area_value_(dg, &child, MAX_COMPENSATION_DEPTH);
        child_value = apply_tactics_q7(NONE, r, &child, child_value);
      }
      if (child_value.high > low) low = child_value.high;
      if (child_value.low > high) high = child_value.low;
    }
    if (
      dg->plain_values[i].low != low || dg->plain_values[i].high != high
    ) {
      dg->plain_values[i] = (table_value) {low, high};
      num_updated++;
    }
  }
  if (verbose) {
    value v = get_dual_graph_value(dg, &(dg->keyspace.keyspace.root), NONE);
    printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
  }
  return num_updated;
}

state dual_graph_low_terminal(dual_graph *dg, const state *origin, tactics ts) {
  if (origin->passes > 1 || (origin->target & ~(origin->player | origin->opponent))) {
    return *origin;
  }
  table_value plain_value;
  table_value forcing_value;
  get_dual_graph_values(dg, origin, MAX_COMPENSATION_DEPTH, &plain_value, &forcing_value);

  // Need to break loops by random navigation
  unsigned int offset = jrand();
  for (int i = 0; i < dg->num_moves; ++i) {
    int j = (i + offset) % dg->num_moves;
    state child = *origin;
    const move_result r = make_move(&child, dg->moves[j]);
    table_value child_plain;
    table_value child_forcing;
    if (r <= TAKE_TARGET) {
      if (ts == NONE) {
        child_plain = score_terminal_q7(r, &child);
        if (plain_value.low == child_plain.high) {
          return child;
        }
      } else {
        child_forcing = score_terminal_q7(r, &child);
        if (forcing_value.low == child_forcing.high) {
          return child;
        }
      }
    } else {
      get_dual_graph_values(dg, &child, MAX_COMPENSATION_DEPTH, &child_plain, &child_forcing);
      if (ts == NONE) {
        child_plain = apply_tactics_q7(ts, r, &child, child_plain);
        if (plain_value.low == child_plain.high) {
          return dual_graph_high_terminal(dg, &child, ts);
        }
      } else {
        child_forcing = apply_tactics_q7(ts, r, &child, child_forcing);
        if (forcing_value.low == child_forcing.high) {
          return dual_graph_high_terminal(dg, &child, ts);
        }
      }
    }
  }
  fprintf(stderr, "Low terminal not found\n");
  exit(EXIT_FAILURE);
  return *origin;
}

state dual_graph_high_terminal(dual_graph *dg, const state *origin, tactics ts) {
  if (origin->passes > 1 || (origin->target & ~(origin->player | origin->opponent))) {
    return *origin;
  }
  table_value plain_value;
  table_value forcing_value;
  get_dual_graph_values(dg, origin, MAX_COMPENSATION_DEPTH, &plain_value, &forcing_value);

  for (int i = 0; i < dg->num_moves; ++i) {
    state child = *origin;
    const move_result r = make_move(&child, dg->moves[i]);
    table_value child_plain;
    table_value child_forcing;
    if (r <= TAKE_TARGET) {
      if (ts == NONE) {
        child_plain = score_terminal_q7(r, &child);
        if (plain_value.high == child_plain.low) {
          return child;
        }
      } else {
        child_forcing = score_terminal_q7(r, &child);
        if (forcing_value.high == child_forcing.low) {
          return child;
        }
      }
    } else {
      get_dual_graph_values(dg, &child, MAX_COMPENSATION_DEPTH, &child_plain, &child_forcing);
      if (ts == NONE) {
        child_plain = apply_tactics_q7(ts, r, &child, child_plain);
        if (plain_value.high == child_plain.low) {
          return dual_graph_high_terminal(dg, &child, ts);
        }
      } else {
        child_forcing = apply_tactics_q7(ts, r, &child, child_forcing);
        if (forcing_value.high == child_forcing.low) {
          return dual_graph_high_terminal(dg, &child, ts);
        }
      }
    }
  }
  fprintf(stderr, "High terminal not found\n");
  exit(EXIT_FAILURE);
  return *origin;
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
