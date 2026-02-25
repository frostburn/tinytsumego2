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
  table_value *value_map = calloc(VALUE_MAP_SIZE, sizeof(table_value));

  // Tactics = NONE
  for (size_t i = 0; i < dg->keyspace.size; ++i) {
    size_t id;
    for (id = 0; id < num_unique; ++id) {
      if (value_map[id].low == dg->plain_values[i].low && value_map[id].high == dg->plain_values[i].high) {
        break;
      }
    }
    if (id >= num_unique) {
      value_map[num_unique++] = dg->plain_values[i];
      if (num_unique >= VALUE_MAP_SIZE) {
        fprintf(stderr, "Too many unique values. Aborting write\n");
        exit(EXIT_FAILURE);
      }
    }
    value_id_t vid = (value_id_t) id;
    total += fwrite(&(vid), sizeof(value_id_t), 1, stream);
  }

  // Tactics = FORCING
  for (size_t i = 0; i < dg->keyspace.size; ++i) {
    size_t id;
    for (id = 0; id < num_unique; ++id) {
      if (value_map[id].low == dg->forcing_values[i].low && value_map[id].high == dg->forcing_values[i].high) {
        break;
      }
    }
    if (id >= num_unique) {
      value_map[num_unique++] = dg->forcing_values[i];
      if (num_unique >= VALUE_MAP_SIZE) {
        fprintf(stderr, "Too many unique values. Aborting write\n");
        exit(EXIT_FAILURE);
      }
    }
    value_id_t vid = (value_id_t) id;
    total += fwrite(&(vid), sizeof(value_id_t), 1, stream);
  }

  for (size_t i = 0; i < VALUE_MAP_SIZE; ++i) {
    value v = table_value_to_value(value_map[i]);
    total += fwrite(&v, sizeof(value), 1, stream);
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

  dgr->plain_value_ids = (value_id_t *)map;
  map += sizeof(value_id_t) * dgr->keyspace.size;

  dgr->forcing_value_ids = (value_id_t *)map;
  map += sizeof(value_id_t) * dgr->keyspace.size;

  dgr->value_map = malloc(VALUE_MAP_SIZE * sizeof(value));
  for (size_t i = 0; i < VALUE_MAP_SIZE; ++i) {
    dgr->value_map[i] = ((value *)map)[i];
  }
  map += sizeof(value) * VALUE_MAP_SIZE;
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
  dgr->value_map = NULL;

  if (dgr->fd >= 0) {
    munmap(dgr->buffer, dgr->sb.st_size);
    close(dgr->fd);

    dgr->buffer = NULL;
    dgr->fd = -1;

    dgr->keyspace.compressor.checkpoints = NULL;
    dgr->keyspace.compressor.deltas = NULL;
    dgr->plain_value_ids = NULL;
    dgr->forcing_value_ids = NULL;
  }
}

void get_dual_graph_reader_values(const dual_graph_reader *dgr, const state *s, int depth, value *plain_value, value *forcing_value) {
  if (!depth) {
    *plain_value = (value){-INFINITY, INFINITY};
    *forcing_value = (value){-INFINITY, INFINITY};
    return;
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    plain_value->low = -INFINITY;
    plain_value->high = -INFINITY;
    forcing_value->low = -INFINITY;
    forcing_value->high = -INFINITY;

    for (int j = 0; j < dgr->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, dgr->moves[j]);
      value child_plain;
      value child_forcing;
      if (r == SECOND_PASS) {
        value simple_area = score_terminal(r, &child);
        float delta = child.ko_threats * KO_THREAT_BONUS;
        child.passes = 0;
        child.ko = 0ULL;
        child.ko_threats = 0;
        get_dual_graph_reader_values(dgr, &child, depth - 1, &child_plain, &child_forcing);
        child_plain.low += delta;
        child_plain.high += delta;
        child_plain = apply_tactics(NONE, r, &child, child_plain);
        // Don't break forcing logic
        child_forcing = simple_area;
      } else if (r <= TAKE_TARGET) {
        child_plain = score_terminal(r, &child);
        child_forcing = child_plain;
      } else {
        get_dual_graph_reader_values(dgr, &child, depth - 1, &child_plain, &child_forcing);
        child_plain = apply_tactics(NONE, r, &child, child_plain);
        child_forcing = apply_tactics(FORCING, r, &child, child_forcing);
      }
      plain_value->low = fmax(plain_value->low, child_plain.high);
      plain_value->high = fmax(plain_value->high, child_plain.low);
      forcing_value->low = fmax(forcing_value->low, child_forcing.high);
      forcing_value->high = fmax(forcing_value->high, child_forcing.low);
    }
    return;
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

  *plain_value = dgr->value_map[dgr->plain_value_ids[key]];
  *forcing_value = dgr->value_map[dgr->forcing_value_ids[key]];
  plain_value->low += delta;
  plain_value->high += delta;
  forcing_value->low += delta;
  forcing_value->high += delta;
}

dual_value get_dual_graph_reader_value(const dual_graph_reader *dgr, const state *s) {
  dual_value result;
  get_dual_graph_reader_values(dgr, s, MAX_COMPENSATION_DEPTH, &result.plain, &result.forcing);
  return result;
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
