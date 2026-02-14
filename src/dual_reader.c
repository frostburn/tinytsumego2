#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tinytsumego2/dual_reader.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"

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

dual_graph_reader load_dual_graph_reader(const char *filename) {
  dual_graph_reader result;
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

  result.plain_value_ids = (value_id_t *)map;
  map += sizeof(value_id_t) * result.keyspace.size;

  result.forcing_value_ids = (value_id_t *)map;
  map += sizeof(value_id_t) * result.keyspace.size;

  result.value_map = malloc(VALUE_MAP_SIZE * sizeof(value));
  for (size_t i = 0; i < VALUE_MAP_SIZE; ++i) {
    result.value_map[i] = ((value *)map)[i];
  }
  map += sizeof(value) * VALUE_MAP_SIZE;

  return result;
}

void unload_dual_graph_reader(dual_graph_reader *dgr) {
  free_tight_keyspace(&(dgr->keyspace.keyspace));

  free(dgr->moves);
  dgr->num_moves = 0;
  dgr->moves = NULL;

  free(dgr->value_map);
  dgr->value_map = NULL;

  munmap(dgr->buffer, dgr->sb.st_size);
  close(dgr->fd);

  dgr->buffer = NULL;
  dgr->fd = -1;

  dgr->keyspace.compressor.checkpoints = NULL;
  dgr->keyspace.compressor.deltas = NULL;
  dgr->plain_value_ids = NULL;
  dgr->forcing_value_ids = NULL;
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
      if (r <= TAKE_TARGET) {
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
