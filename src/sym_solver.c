#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "jkiss/jkiss.h"
#include "tinytsumego2/sym_solver.h"

sym_graph create_sym_graph(const state *root) {
  if (root->ko || root->passes) {
    fprintf(stderr, "The root state may not have an active ko or previous passes\n");
    exit(EXIT_FAILURE);
  }

  sym_graph sg = {0};

  sg.keyspace = create_symmetric_keyspace(root);

  sg.moves = moves_of(root, &sg.num_moves);

  sg.plain_values = malloc(sg.keyspace.size * sizeof(table_value));
  sg.forcing_values = malloc(sg.keyspace.size * sizeof(table_value));

  for (size_t i = 0; i < sg.keyspace.size; ++i) {
    sg.plain_values[i] = MAX_RANGE_Q7;
    sg.forcing_values[i] = MAX_RANGE_Q7;
  }

  return sg;
}

sym_graph* allocate_sym_graph(const state *root) {
  sym_graph *result = malloc(sizeof(sym_graph));
  *result = create_sym_graph(root);
  return result;
}

void get_sym_graph_values(sym_graph *sg, const state *s, int depth, table_value *plain_value, table_value *forcing_value) {
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

    for (int j = 0; j < sg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, sg->moves[j]);
      table_value child_plain;
      table_value child_forcing;
      if (r <= TAKE_TARGET) {
        child_plain = score_terminal_q7(r, &child);
        child_forcing = child_plain;
      } else {
        get_sym_graph_values(sg, &child, depth - 1, &child_plain, &child_forcing);
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
    key = to_symmetric_key(&(sg->keyspace), &c);
  } else {
    key = to_symmetric_key(&(sg->keyspace), s);
  }
  *plain_value = sg->plain_values[key];
  *forcing_value = sg->forcing_values[key];
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

value get_sym_graph_value(sym_graph *sg, const state *s, tactics ts) {
  table_value plain_value;
  table_value forcing_value;
  get_sym_graph_values(sg, s, MAX_COMPENSATION_DEPTH, &plain_value, &forcing_value);
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

bool iterate_sym_graph(sym_graph *sg, bool verbose) {
  size_t num_updated = 0;
  for (size_t k = 0; k < sg->keyspace.fast_size; ++k) {
    if (!was_symmetric_legal(&(sg->keyspace), k)) {
      continue;
    }
    size_t i = remap_fast_key(&(sg->keyspace), k);
    // Don't evaluate if the range cannot be tightened
    if (sg->plain_values[i].low == sg->plain_values[i].high && sg->forcing_values[i].low == sg->forcing_values[i].high) {
      continue;
    }
    // Perform negamax (with memory to break delay shuffling)
    score_q7_t plain_low = sg->plain_values[i].low;
    score_q7_t plain_high = SCORE_Q7_MIN;
    score_q7_t forcing_low = sg->forcing_values[i].low;
    score_q7_t forcing_high = SCORE_Q7_MIN;

    state parent = from_fast_key(&(sg->keyspace), k);
    for (int j = 0; j < sg->num_moves; ++j) {
      state child = parent;
      const move_result r = make_move(&child, sg->moves[j]);
      table_value child_plain;
      table_value child_forcing;
      if (r <= TAKE_TARGET) {
        child_plain = score_terminal_q7(r, &child);
        child_forcing = child_plain;
      } else {
        get_sym_graph_values(sg, &child, MAX_COMPENSATION_DEPTH, &child_plain, &child_forcing);
        child_plain = apply_tactics_q7(NONE, r, &child, child_plain);
        child_forcing = apply_tactics_q7(FORCING, r, &child, child_forcing);
      }
      if (child_plain.high > plain_low) plain_low = child_plain.high;
      if (child_plain.low > plain_high) plain_high = child_plain.low;
      if (child_forcing.high > forcing_low) forcing_low = child_forcing.high;
      if (child_forcing.low > forcing_high) forcing_high = child_forcing.low;
    }
    if (
      sg->plain_values[i].low != plain_low || sg->plain_values[i].high != plain_high ||
      sg->forcing_values[i].low != forcing_low || sg->forcing_values[i].high != forcing_high
    ) {
      sg->plain_values[i] = (table_value) {plain_low, plain_high};
      sg->forcing_values[i] = (table_value) {forcing_low, forcing_high};
      num_updated++;
    }
  }
  if (verbose) {
    value v = get_sym_graph_value(sg, &(sg->keyspace.root), NONE);
    printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
  }
  return num_updated;
}

table_value get_sym_graph_area_value_(sym_graph *sg, const state *s, int depth) {
  if (!depth) {
    return MAX_RANGE_Q7;
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    score_q7_t low = SCORE_Q7_MIN;
    score_q7_t high = SCORE_Q7_MIN;

    for (int j = 0; j < sg->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, sg->moves[j]);
      table_value child_value;
      if (r == SECOND_PASS) {
        // For the most part true area scoring agrees with simple area scoring,
        // but removing ko-threats after two passes makes
        // "Bent Four in the Corner is Dead"-type cases into dead shapes instead of seki.
        score_q7_t delta = child.ko_threats * KO_THREAT_Q7;
        child.ko = 0ULL;
        child.ko_threats = 0;
        child.passes = 0;
        child_value = get_sym_graph_area_value_(sg, &child, depth - 1);
        child_value.low += delta;
        child_value.high += delta;
        child_value = apply_tactics_q7(NONE, r, &child, child_value);
      } else if (r <= TAKE_TARGET) {
        child_value = score_terminal_q7(r, &child);
      } else {
        child_value = get_sym_graph_area_value_(sg, &child, depth - 1);
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
    key = to_symmetric_key(&(sg->keyspace), &c);
  } else {
    key = to_symmetric_key(&(sg->keyspace), s);
  }
  table_value v = sg->plain_values[key];
  if (v.low != SCORE_Q7_MIN) {
    v.low += delta;
  }
  if (v.high != SCORE_Q7_MAX) {
    v.high += delta;
  }
  return v;
}

value get_sym_graph_area_value(sym_graph *sg, const state *s) {
  return table_value_to_value(get_sym_graph_area_value_(sg, s, MAX_COMPENSATION_DEPTH));
}

bool area_iterate_sym_graph(sym_graph *sg, bool verbose) {
  size_t num_updated = 0;
  for (size_t k = 0; k < sg->keyspace.fast_size; ++k) {
    if (!was_symmetric_legal(&(sg->keyspace), k)) {
      continue;
    }
    size_t i = remap_fast_key(&(sg->keyspace), k);

    // Perform negamax
    score_q7_t low = SCORE_Q7_MIN;
    score_q7_t high = SCORE_Q7_MIN;

    state parent = from_fast_key(&(sg->keyspace), k);
    for (int j = 0; j < sg->num_moves; ++j) {
      state child = parent;
      const move_result r = make_move(&child, sg->moves[j]);
      table_value child_value;
      if (r <= TAKE_TARGET) {
        child_value = score_terminal_q7(r, &child);
      } else {
        child_value = get_sym_graph_area_value_(sg, &child, MAX_COMPENSATION_DEPTH);
        child_value = apply_tactics_q7(NONE, r, &child, child_value);
      }
      if (child_value.high > low) low = child_value.high;
      if (child_value.low > high) high = child_value.low;
    }
    if (
      sg->plain_values[i].low != low || sg->plain_values[i].high != high
    ) {
      sg->plain_values[i] = (table_value) {low, high};
      num_updated++;
    }
  }
  if (verbose) {
    value v = get_sym_graph_value(sg, &(sg->keyspace.root), NONE);
    printf("%zu nodes updated. Root value = %f, %f\n", num_updated, v.low, v.high);
  }
  return num_updated;
}

state sym_graph_low_terminal(sym_graph *sg, const state *origin, tactics ts) {
  if (origin->passes > 1 || (origin->target & ~(origin->player | origin->opponent))) {
    return *origin;
  }
  table_value plain_value;
  table_value forcing_value;
  get_sym_graph_values(sg, origin, MAX_COMPENSATION_DEPTH, &plain_value, &forcing_value);

  // Need to break loops by random navigation
  unsigned int offset = jrand();
  for (int i = 0; i < sg->num_moves; ++i) {
    int j = (i + offset) % sg->num_moves;
    state child = *origin;
    const move_result r = make_move(&child, sg->moves[j]);
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
      get_sym_graph_values(sg, &child, MAX_COMPENSATION_DEPTH, &child_plain, &child_forcing);
      if (ts == NONE) {
        child_plain = apply_tactics_q7(ts, r, &child, child_plain);
        if (plain_value.low == child_plain.high) {
          return sym_graph_high_terminal(sg, &child, ts);
        }
      } else {
        child_forcing = apply_tactics_q7(ts, r, &child, child_forcing);
        if (forcing_value.low == child_forcing.high) {
          return sym_graph_high_terminal(sg, &child, ts);
        }
      }
    }
  }
  fprintf(stderr, "Low terminal not found\n");
  exit(EXIT_FAILURE);
  return *origin;
}

state sym_graph_high_terminal(sym_graph *sg, const state *origin, tactics ts) {
  if (origin->passes > 1 || (origin->target & ~(origin->player | origin->opponent))) {
    return *origin;
  }
  table_value plain_value;
  table_value forcing_value;
  get_sym_graph_values(sg, origin, MAX_COMPENSATION_DEPTH, &plain_value, &forcing_value);

  for (int i = 0; i < sg->num_moves; ++i) {
    state child = *origin;
    const move_result r = make_move(&child, sg->moves[i]);
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
      get_sym_graph_values(sg, &child, MAX_COMPENSATION_DEPTH, &child_plain, &child_forcing);
      if (ts == NONE) {
        child_plain = apply_tactics_q7(ts, r, &child, child_plain);
        if (plain_value.high == child_plain.low) {
          return sym_graph_high_terminal(sg, &child, ts);
        }
      } else {
        child_forcing = apply_tactics_q7(ts, r, &child, child_forcing);
        if (forcing_value.high == child_forcing.low) {
          return sym_graph_high_terminal(sg, &child, ts);
        }
      }
    }
  }
  fprintf(stderr, "High terminal not found\n");
  exit(EXIT_FAILURE);
  return *origin;
}

void free_sym_graph(sym_graph *sg) {
  free_symmetric_keyspace(&(sg->keyspace));

  free(sg->moves);
  sg->num_moves = 0;
  sg->moves = NULL;

  free(sg->plain_values);
  sg->plain_values = NULL;
  free(sg->forcing_values);
  sg->forcing_values = NULL;
}
