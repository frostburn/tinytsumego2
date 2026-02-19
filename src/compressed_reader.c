#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tinytsumego2/compressed_reader.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"
#include "tinytsumego2/keyspace.h"

size_t write_compressed_graph(const compressed_graph *restrict cg, FILE *restrict stream) {
  size_t total = fwrite(&(cg->keyspace.keyspace.root), sizeof(state), 1, stream);
  total += fwrite(&(cg->keyspace.compressor.num_checkpoints), sizeof(size_t), 1, stream);
  total += fwrite(cg->keyspace.compressor.checkpoints, sizeof(size_t), cg->keyspace.compressor.num_checkpoints, stream);
  total += fwrite(&(cg->keyspace.compressor.uncompressed_size), sizeof(size_t), 1, stream);
  total += fwrite(cg->keyspace.compressor.deltas, sizeof(unsigned char), cg->keyspace.compressor.uncompressed_size, stream);
  total += fwrite(&(cg->keyspace.compressor.size), sizeof(size_t), 1, stream);
  total += fwrite(&(cg->keyspace.compressor.factor), sizeof(double), 1, stream);
  total += fwrite(&(cg->keyspace.prefix_m), sizeof(size_t), 1, stream);
  total += fwrite(&(cg->keyspace.size), sizeof(size_t), 1, stream);
  total += fwrite(&(cg->num_moves), sizeof(int), 1, stream);
  total += fwrite(cg->moves, sizeof(stones_t), cg->num_moves, stream);

  // The actual reader uses float values but we can write using a temporary fixed-point array
  size_t num_unique = 0;
  table_value *value_map = calloc(VALUE_MAP_SIZE, sizeof(table_value));

  // Tactics = NONE
  for (size_t i = 0; i < cg->keyspace.size; ++i) {
    size_t id;
    for (id = 0; id < num_unique; ++id) {
      if (value_map[id].low == cg->values[i].low && value_map[id].high == cg->values[i].high) {
        break;
      }
    }
    if (id >= num_unique) {
      value_map[num_unique++] = cg->values[i];
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

compressed_graph_reader load_compressed_graph_reader(const char *filename) {
  compressed_graph_reader result;
  char *map = file_to_mmap(filename, &(result.sb), &(result.fd));

  result.buffer = map;

  state root = ((state *)map)[0];
  map += sizeof(state);
  result.keyspace.keyspace = create_tight_keyspace(&root, true);

  result.keyspace.compressor.num_checkpoints = ((size_t*) map)[0];
  map += sizeof(size_t);

  // Memory map aux data to save RAM
  result.keyspace.compressor.checkpoints = (size_t *)map;
  map += sizeof(size_t) * result.keyspace.compressor.num_checkpoints;

  result.keyspace.compressor.uncompressed_size = ((size_t*) map)[0];
  map += sizeof(size_t);

  result.keyspace.compressor.deltas = (unsigned char *)map;
  map += sizeof(unsigned char) * result.keyspace.compressor.uncompressed_size;

  result.keyspace.compressor.size = ((size_t*) map)[0];
  map += sizeof(size_t);

  result.keyspace.compressor.factor = ((double*) map)[0];
  map += sizeof(double);

  result.keyspace.prefix_m = ((size_t*) map)[0];
  map += sizeof(size_t);

  result.keyspace.size = ((size_t*) map)[0];
  map += sizeof(size_t);

  result.num_moves = ((int*) map)[0];
  map += sizeof(int);

  result.moves = malloc(result.num_moves * sizeof(stones_t));
  for (int i = 0; i < result.num_moves; ++i) {
    result.moves[i] = ((stones_t *) map)[i];
  }
  map += result.num_moves * sizeof(stones_t);

  result.value_ids = (value_id_t *)map;
  map += sizeof(value_id_t) * result.keyspace.size;

  result.value_map = malloc(VALUE_MAP_SIZE * sizeof(value));
  for (size_t i = 0; i < VALUE_MAP_SIZE; ++i) {
    result.value_map[i] = ((value *)map)[i];
  }
  map += sizeof(value) * VALUE_MAP_SIZE;

  return result;
}

void unload_compressed_graph_reader(compressed_graph_reader *cgr) {
  free_tight_keyspace(&(cgr->keyspace.keyspace));

  free(cgr->moves);
  cgr->num_moves = 0;
  cgr->moves = NULL;

  free(cgr->value_map);
  cgr->value_map = NULL;

  munmap(cgr->buffer, cgr->sb.st_size);
  close(cgr->fd);

  cgr->buffer = NULL;
  cgr->fd = -1;

  cgr->keyspace.compressor.checkpoints = NULL;
  cgr->keyspace.compressor.deltas = NULL;
  cgr->value_ids = NULL;
}

value get_compressed_graph_reader_value_(const compressed_graph_reader *cgr, const state *s, int depth) {
  if (!depth) {
    return (value){-INFINITY, INFINITY};
  }
  if (s->passes || s->ko) {
    // Compensate for keyspace tightness using negamax
    float low = -INFINITY;
    float high = -INFINITY;

    for (int j = 0; j < cgr->num_moves; ++j) {
      state child = *s;
      const move_result r = make_move(&child, cgr->moves[j]);
      value child_value;
      if (r <= TAKE_TARGET) {
        child_value = score_terminal(r, &child);
      } else {
        child_value = get_compressed_graph_reader_value_(cgr, &child, depth - 1);
        child_value = apply_tactics(NONE, r, &child, child_value);
      }
      low = fmax(low, child_value.high);
      high = fmax(high, child_value.low);
    }
    return (value) {low, high};
  }

  float delta = 0;
  size_t key;

  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    key = to_compressed_key(&(cgr->keyspace), &c);
    delta = -2 * BUTTON_BONUS;
  } else {
    key = to_compressed_key(&(cgr->keyspace), s);
  }

  value v = cgr->value_map[cgr->value_ids[key]];
  return (value) {v.low + delta, v.high + delta};
}

value get_compressed_graph_reader_value(const compressed_graph_reader *cgr, const state *s) {
  return get_compressed_graph_reader_value_(cgr, s, MAX_COMPENSATION_DEPTH);
}

compressed_graph_reader* allocate_compressed_graph_reader(const char *filename) {
  compressed_graph_reader *result = malloc(sizeof(compressed_graph_reader));
  *result = load_compressed_graph_reader(filename);
  return result;
}

stones_t* compressed_graph_reader_python_stuff(compressed_graph_reader *cgr, state *root, int *num_moves) {
  *root = cgr->keyspace.keyspace.root;
  *num_moves = cgr->num_moves;
  return cgr->moves;
}

move_info* compressed_graph_reader_move_infos(const compressed_graph_reader *cgr, const state *s, int *num_move_infos) {
  move_info *result = malloc(cgr->num_moves * sizeof(move_info));
  *num_move_infos = 0;

  // Strip aesthetics
  state parent = *s;
  parent.button = abs(parent.button);
  size_t key = to_tight_key_fast(&(cgr->keyspace.keyspace), &parent);
  parent = from_tight_key_fast(&(cgr->keyspace.keyspace), key);
  parent.button = s->button;

  value v = get_compressed_graph_reader_value(cgr, &parent);

  float lows_high = -INFINITY;
  float highs_low = -INFINITY;
  value *child_values = malloc(cgr->num_moves * sizeof(value));
  bool *forcing_flags = malloc(cgr->num_moves * sizeof(bool));
  for (int i = 0; i < cgr->num_moves; ++i) {
    state child = parent;
    const move_result r = make_move(&child, cgr->moves[i]);
    forcing_flags[i] = false;
    if (r <= TAKE_TARGET) {
      child_values[i] = score_terminal(r, &child);
    } else {
      value child_value = get_compressed_graph_reader_value(cgr, &child);
      child_values[i] = apply_tactics(NONE, r, &child, child_value);
      // Check if the opponent can pass as a response to a high move
      if (v.high == child_values[i].low) {
        state grand_child = child;
        const move_result gr = make_move(&grand_child, pass());
        value grand_child_value;
        if (gr <= TAKE_TARGET) {
          grand_child_value = score_terminal(gr, &grand_child);
        } else {
          grand_child_value = get_compressed_graph_reader_value(cgr, &grand_child);
          grand_child_value = apply_tactics(NONE, gr, &grand_child, grand_child_value);
        }
        if (child_value.low > grand_child_value.high) {
          // Passing doesn't work. Forcing successful
          forcing_flags[i] = true;
        }
      }
    }
    if (v.low == child_values[i].high) {
      lows_high = fmax(lows_high, child_values[i].low);
    }
    if (v.high == child_values[i].low) {
      highs_low = fmax(highs_low, child_values[i].high);
    }
  }

  for (int i = 0; i < cgr->num_moves; ++i) {
    if (isnan(child_values[i].low)) {
      continue;
    }
    result[(*num_move_infos)++] = (move_info) {
      s->wide ? coords_of_16(cgr->moves[i]) : coords_of(cgr->moves[i]),
      child_values[i].high - v.low,
      child_values[i].low - v.high,
      v.low == child_values[i].high && lows_high == child_values[i].low,
      v.high == child_values[i].low && highs_low == child_values[i].high,
      forcing_flags[i],
    };
  }

  free(child_values);
  free(forcing_flags);

  return realloc(result, *num_move_infos * sizeof(move_info));
}
