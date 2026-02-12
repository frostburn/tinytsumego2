#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tinytsumego2/complete_reader.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"

size_t write_complete_graph(const complete_graph *restrict cg, FILE *restrict stream) {
  size_t total = fwrite(&(cg->keyspace.root), sizeof(state), 1, stream);
  total += fwrite(&(cg->keyspace.symmetric_threats), sizeof(bool), 1, stream);
  total += fwrite(&(cg->tactics), sizeof(tactics), 1, stream);
  total += fwrite(&(cg->num_moves), sizeof(int), 1, stream);
  total += fwrite(cg->moves, sizeof(stones_t), cg->num_moves, stream);

  // The actual reader uses float values but we can write using a temporary fixed-point array
  size_t num_unique = 0;
  table_value *value_map = calloc(VALUE_MAP_SIZE, sizeof(table_value));

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

complete_graph_reader load_complete_graph_reader(const char *filename) {
  complete_graph_reader result;
  char *map = file_to_mmap(filename, &(result.sb), &(result.fd));

  result.buffer = map;

  state root = ((state *)map)[0];
  map += sizeof(state);
  bool symmetric_threats = ((bool *)map)[0];
  map += sizeof(bool);
  result.keyspace = create_tight_keyspace(&root, symmetric_threats);

  result.tactics = ((tactics*) map)[0];
  map += sizeof(tactics);

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

void unload_complete_graph_reader(complete_graph_reader *cgr) {
  free_tight_keyspace(&(cgr->keyspace));

  free(cgr->moves);
  cgr->num_moves = 0;
  cgr->moves = NULL;

  free(cgr->value_map);
  cgr->value_map = NULL;

  munmap(cgr->buffer, cgr->sb.st_size);
  close(cgr->fd);

  cgr->buffer = NULL;
  cgr->fd = -1;

  cgr->value_ids = NULL;
}

value get_complete_graph_reader_value_(const complete_graph_reader *cgr, const state *s, int depth) {
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
        child_value = apply_tactics(
          cgr->tactics,
          r,
          &child,
          get_complete_graph_reader_value_(cgr, &child, depth - 1)
        );
      }
      low = fmax(low, child_value.high);
      high = fmax(high, child_value.low);
    }
    return (value){low, high};
  }

  float delta = 0;
  value_id_t id;

  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    size_t key = to_tight_key_fast(&(cgr->keyspace), &c);
    id = cgr->value_ids[key];
    delta = -2 * BUTTON_BONUS;
  } else {
    size_t key = to_tight_key_fast(&(cgr->keyspace), s);
    id = cgr->value_ids[key];
  }

  value v = cgr->value_map[id];
  return (value){
    v.low + delta,
    v.high + delta
  };
}

value get_complete_graph_reader_value(const complete_graph_reader *cgr, const state *s) {
  return get_complete_graph_reader_value_(cgr, s, MAX_COMPENSATION_DEPTH);
}
