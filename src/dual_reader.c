#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "jkiss/jkiss.h"
#include "tinytsumego2/dual_reader.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"
#include "tinytsumego2/keyspace.h"

size_t write_dual_graph(const dual_graph *restrict dg, FILE *restrict stream) {
  size_t total = fwrite(&(dg->keyspace.keyspace.root), sizeof(state), 1, stream);
  total += fwrite(&(dg->keyspace.compressor.num_checkpoints), sizeof(size_t), 1, stream);
  total += fwrite(dg->keyspace.compressor.checkpoints, sizeof(size_t), dg->keyspace.compressor.num_checkpoints, stream);
  total += fwrite(&(dg->keyspace.compressor.uncompressed_size), sizeof(size_t), 1, stream);
  total += fwrite(dg->keyspace.compressor.deltas, sizeof(unsigned char), dg->keyspace.compressor.uncompressed_size, stream);
  total += fwrite(&(dg->keyspace.compressor.size), sizeof(size_t), 1, stream);
  total += fwrite(&(dg->keyspace.compressor.factor), sizeof(double), 1, stream);
  total += fwrite(&(dg->keyspace.prefix_m), sizeof(size_t), 1, stream);
  total += fwrite(&(dg->keyspace.size), sizeof(size_t), 1, stream);
  total += fwrite(&(dg->num_moves), sizeof(int), 1, stream);
  total += fwrite(dg->moves, sizeof(stones_t), dg->num_moves, stream);

  // The actual reader uses float values but we can write using a temporary fixed-point array
  size_t num_unique = 0;
  size_t capacity = 128;
  score_q7_t *value_map = malloc(capacity * 4 * sizeof(score_q7_t));

  for (size_t i = 0; i < dg->keyspace.size; ++i) {
    size_t id;
    for (id = 0; id < num_unique; ++id) {
      if (
        value_map[4*id + 0] == dg->plain_values[i].low &&
        value_map[4*id + 1] == dg->plain_values[i].high &&
        value_map[4*id + 2] == dg->forcing_values[i].low &&
        value_map[4*id + 3] == dg->forcing_values[i].high
      ) {
        break;
      }
    }
    if (id >= num_unique) {
      if (num_unique >= capacity) {
        capacity <<= 1;
        value_map = realloc(value_map, capacity * 4 * sizeof(score_q7_t));
      }
      value_map[4*num_unique + 0] = dg->plain_values[i].low;
      value_map[4*num_unique + 1] = dg->plain_values[i].high;
      value_map[4*num_unique + 2] = dg->forcing_values[i].low;
      value_map[4*num_unique + 3] = dg->forcing_values[i].high;
      num_unique++;
    }
    dual_value_id_t vid = (dual_value_id_t) id;
    total += fwrite(&(vid), sizeof(dual_value_id_t), 1, stream);
  }

  total += fwrite(&num_unique, sizeof(size_t), 1, stream);
  for (size_t i = 0; i < num_unique; ++i) {
    dual_value v = (dual_value){
      {score_q7_to_float(value_map[4*i + 0]), score_q7_to_float(value_map[4*i + 1])},
      {score_q7_to_float(value_map[4*i + 2]), score_q7_to_float(value_map[4*i + 3])},
    };
    total += fwrite(&v, sizeof(dual_value), 1, stream);
  }

  free(value_map);

  return total;
}

void unbuffer_dual_graph_reader(dual_graph_reader *dgr) {
  char *map = dgr->buffer;

  state root = ((state *)map)[0];
  map += sizeof(state);
  dgr->keyspace.keyspace = create_tight_keyspace(&root, true);

  dgr->keyspace.compressor.num_checkpoints = ((size_t*) map)[0];
  map += sizeof(size_t);

  // Memory map aux data to save RAM
  dgr->keyspace.compressor.checkpoints = (size_t *)map;
  map += sizeof(size_t) * dgr->keyspace.compressor.num_checkpoints;

  dgr->keyspace.compressor.uncompressed_size = ((size_t*) map)[0];
  map += sizeof(size_t);

  dgr->keyspace.compressor.deltas = (unsigned char *)map;
  map += sizeof(unsigned char) * dgr->keyspace.compressor.uncompressed_size;

  dgr->keyspace.compressor.size = ((size_t*) map)[0];
  map += sizeof(size_t);

  dgr->keyspace.compressor.factor = ((double*) map)[0];
  map += sizeof(double);

  dgr->keyspace.prefix_m = ((size_t*) map)[0];
  map += sizeof(size_t);

  dgr->keyspace.size = ((size_t*) map)[0];
  map += sizeof(size_t);

  dgr->num_moves = ((int*) map)[0];
  map += sizeof(int);

  dgr->moves = malloc(dgr->num_moves * sizeof(stones_t));
  for (int i = 0; i < dgr->num_moves; ++i) {
    dgr->moves[i] = ((stones_t *) map)[i];
  }
  map += dgr->num_moves * sizeof(stones_t);

  dgr->value_ids = (dual_value_id_t *)map;
  map += sizeof(dual_value_id_t) * dgr->keyspace.size;

  dgr->value_map_size = ((size_t*) map)[0];
  map += sizeof(size_t);

  dgr->value_map = malloc(dgr->value_map_size * sizeof(dual_value));
  for (size_t i = 0; i < dgr->value_map_size; ++i) {
    dgr->value_map[i] = ((dual_value *)map)[i];
  }
  map += sizeof(dual_value) * dgr->value_map_size;
}

dual_graph_reader load_dual_graph_reader(const char *filename) {
  dual_graph_reader result;
  char *map = file_to_mmap(filename, &(result.sb), &(result.fd));

  result.buffer = map;

  unbuffer_dual_graph_reader(&result);

  return result;
}

void unload_dual_graph_reader(dual_graph_reader *dgr) {
  free_tight_keyspace(&(dgr->keyspace.keyspace));

  free(dgr->moves);
  dgr->num_moves = 0;
  dgr->moves = NULL;

  free(dgr->value_map);
  dgr->value_map_size = 0;
  dgr->value_map = NULL;

  if (dgr->fd >= 0) {
    munmap(dgr->buffer, dgr->sb.st_size);
    close(dgr->fd);

    dgr->buffer = NULL;
    dgr->fd = -1;

    dgr->keyspace.compressor.checkpoints = NULL;
    dgr->keyspace.compressor.deltas = NULL;
    dgr->value_ids = NULL;
  }
}

dual_value get_dual_graph_reader_value_(const dual_graph_reader *dgr, const state *s, int depth) {
  if (!depth) {
    return (dual_value){{-INFINITY, INFINITY}, {-INFINITY, INFINITY}};
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    dual_value v = (dual_value){{-INFINITY, -INFINITY}, {-INFINITY, -INFINITY}};

    for (int j = 0; j < dgr->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, dgr->moves[j]);
      dual_value child_value;
      if (r == SECOND_PASS) {
        value simple_area = score_terminal(r, &child);
        float delta = child.ko_threats * KO_THREAT_BONUS;
        child.passes = 0;
        child.ko = 0ULL;
        child.ko_threats = 0;
        child_value = get_dual_graph_reader_value_(dgr, &child, depth - 1);
        child_value.plain.low += delta;
        child_value.plain.high += delta;
        child_value.plain = apply_tactics(NONE, r, &child, child_value.plain);
        // Don't break forcing logic
        child_value.forcing = simple_area;
      } else if (r <= TAKE_TARGET) {
        child_value.plain = score_terminal(r, &child);
        child_value.forcing = child_value.plain;
      } else {
        child_value = get_dual_graph_reader_value_(dgr, &child, depth - 1);
        child_value.plain = apply_tactics(NONE, r, &child, child_value.plain);
        child_value.forcing = apply_tactics(FORCING, r, &child, child_value.forcing);
      }
      v.plain.low = fmax(v.plain.low, child_value.plain.high);
      v.plain.high = fmax(v.plain.high, child_value.plain.low);
      v.forcing.low = fmax(v.forcing.low, child_value.forcing.high);
      v.forcing.high = fmax(v.forcing.high, child_value.forcing.low);
    }
    return v;
  }

  float delta = 0;
  size_t key;

  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    key = to_compressed_key(&(dgr->keyspace), &c);
    delta = -2 * BUTTON_BONUS;
  } else {
    key = to_compressed_key(&(dgr->keyspace), s);
  }

  dual_value v = dgr->value_map[dgr->value_ids[key]];
  v.plain.low += delta;
  v.plain.high += delta;
  v.forcing.low += delta;
  v.forcing.high += delta;
  return v;
}

dual_value get_dual_graph_reader_value(const dual_graph_reader *dgr, const state *s) {
  return get_dual_graph_reader_value_(dgr, s, MAX_COMPENSATION_DEPTH);
}

dual_graph_reader* allocate_dual_graph_reader(const char *filename) {
  dual_graph_reader *result = malloc(sizeof(dual_graph_reader));
  *result = load_dual_graph_reader(filename);
  return result;
}

stones_t* dual_graph_reader_python_stuff(dual_graph_reader *dgr, state *root, int *num_moves) {
  *root = dgr->keyspace.keyspace.root;
  *num_moves = dgr->num_moves;
  return dgr->moves;
}

state strip_aesthetics(const dual_graph_reader *dgr, const state *s) {
  state ss = *s;
  ss.button = abs(ss.button);
  size_t key = to_tight_key_fast(&(dgr->keyspace.keyspace), &ss);
  ss = from_tight_key_fast(&(dgr->keyspace.keyspace), key);
  ss.ko = s->ko;
  ss.passes = s->passes;
  ss.button = s->button;

  return ss;
}

move_info* dual_graph_reader_move_infos(const dual_graph_reader *dgr, const state *s, int *num_move_infos) {
  move_info *result = malloc(dgr->num_moves * sizeof(move_info));
  *num_move_infos = 0;

  state parent = strip_aesthetics(dgr, s);

  dual_value v = get_dual_graph_reader_value(dgr, &parent);

  float lows_high = -INFINITY;
  float highs_low = -INFINITY;
  dual_value *child_values = malloc(dgr->num_moves * sizeof(dual_value));
  for (int i = 0; i < dgr->num_moves; ++i) {
    state child = parent;
    const move_result r = make_move(&child, dgr->moves[i]);
    if (r == SECOND_PASS) {
      value simple_area = score_terminal(r, &child);
      float delta = child.ko_threats * KO_THREAT_BONUS;
      child.passes = 0;
      child.ko = 0ULL;
      child.ko_threats = 0;
      child_values[i] = get_dual_graph_reader_value(dgr, &child);
      child_values[i].plain.low += delta;
      child_values[i].plain.high += delta;
      child_values[i].plain = apply_tactics(NONE, r, &child, child_values[i].plain);
      // Don't break forcing logic
      child_values[i].forcing = simple_area;
    } else if (r <= TAKE_TARGET) {
      child_values[i].plain = score_terminal(r, &child);
      child_values[i].forcing = child_values[i].plain;
    } else {
      child_values[i] = get_dual_graph_reader_value(dgr, &child);
      child_values[i].plain = apply_tactics(NONE, r, &child, child_values[i].plain);
      child_values[i].forcing = apply_tactics(FORCING, r, &child, child_values[i].forcing);
    }
    if (v.plain.low == child_values[i].plain.high) {
      lows_high = fmax(lows_high, child_values[i].plain.low);
    }
    if (v.plain.high == child_values[i].plain.low) {
      highs_low = fmax(highs_low, child_values[i].plain.high);
    }
  }

  for (int i = 0; i < dgr->num_moves; ++i) {
    if (isnan(child_values[i].plain.low)) {
      continue;
    }
    result[(*num_move_infos)++] = (move_info) {
      s->wide ? coords_of_16(dgr->moves[i]) : coords_of(dgr->moves[i]),
      child_values[i].plain.high - v.plain.low,
      child_values[i].plain.low - v.plain.high,
      v.plain.low == child_values[i].plain.high && lows_high == child_values[i].plain.low,
      v.plain.high == child_values[i].plain.low && highs_low == child_values[i].plain.high,
      v.forcing.high == child_values[i].forcing.low,
    };
  }

  free(child_values);

  return realloc(result, *num_move_infos * sizeof(move_info));
}

state dual_graph_reader_high_terminal_(dual_graph_reader *dgr, const state *origin, tactics ts);

state dual_graph_reader_low_terminal_(dual_graph_reader *dgr, const state *origin, tactics ts) {
  if (origin->passes > 1 || (origin->target & ~(origin->player | origin->opponent))) {
    return *origin;
  }
  dual_value v = get_dual_graph_reader_value(dgr, origin);

  // Need to break loops by random navigation
  unsigned int offset = jrand();
  for (int i = 0; i < dgr->num_moves; ++i) {
    int j = (i + offset) % dgr->num_moves;
    state child = *origin;
    const move_result r = make_move(&child, dgr->moves[j]);
    dual_value child_value;
    if (r <= TAKE_TARGET) {
      if (ts == NONE) {
        child_value.plain = score_terminal(r, &child);
        if (v.plain.low == child_value.plain.high) {
          return child;
        }
      } else {
        child_value.forcing = score_terminal(r, &child);
        if (v.forcing.low == child_value.forcing.high) {
          return child;
        }
      }
    } else {
      child_value = get_dual_graph_reader_value(dgr, &child);
      if (ts == NONE) {
        child_value.plain = apply_tactics(ts, r, &child, child_value.plain);
        if (v.plain.low == child_value.plain.high) {
          return dual_graph_reader_high_terminal_(dgr, &child, ts);
        }
      } else {
        child_value.forcing = apply_tactics(ts, r, &child, child_value.forcing);
        if (v.forcing.low == child_value.forcing.high) {
          return dual_graph_reader_high_terminal_(dgr, &child, ts);
        }
      }
    }
  }
  fprintf(stderr, "Low terminal not found\n");
  exit(EXIT_FAILURE);
  return *origin;
}

state dual_graph_reader_high_terminal_(dual_graph_reader *dgr, const state *origin, tactics ts) {
  if (origin->passes > 1 || (origin->target & ~(origin->player | origin->opponent))) {
    return *origin;
  }
  dual_value v = get_dual_graph_reader_value(dgr, origin);

  for (int i = 0; i < dgr->num_moves; ++i) {
    state child = *origin;
    const move_result r = make_move(&child, dgr->moves[i]);
    dual_value child_value;
    if (r <= TAKE_TARGET) {
      if (ts == NONE) {
        child_value.plain = score_terminal(r, &child);
        if (v.plain.high == child_value.plain.low) {
          return child;
        }
      } else {
        child_value.forcing = score_terminal(r, &child);
        if (v.forcing.high == child_value.forcing.low) {
          return child;
        }
      }
    } else {
      child_value = get_dual_graph_reader_value(dgr, &child);
      if (ts == NONE) {
        child_value.plain = apply_tactics(ts, r, &child, child_value.plain);
        if (v.plain.high == child_value.plain.low) {
          return dual_graph_reader_low_terminal_(dgr, &child, ts);
        }
      } else {
        child_value.forcing = apply_tactics(ts, r, &child, child_value.forcing);
        if (v.forcing.high == child_value.forcing.low) {
          return dual_graph_reader_low_terminal_(dgr, &child, ts);
        }
      }
    }
  }
  fprintf(stderr, "High terminal not found\n");
  exit(EXIT_FAILURE);
  return *origin;
}

state dual_graph_reader_low_terminal(dual_graph_reader *dgr, const state *origin, tactics ts) {
  state o = strip_aesthetics(dgr, origin);
  return dual_graph_reader_low_terminal_(dgr, &o, ts);
}

state dual_graph_reader_high_terminal(dual_graph_reader *dgr, const state *origin, tactics ts) {
  state o = strip_aesthetics(dgr, origin);
  return dual_graph_reader_high_terminal_(dgr, &o, ts);
}
