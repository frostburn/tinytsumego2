#include "tinytsumego2/complete_reader.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

size_t write_complete_graph(const complete_graph *restrict cg, FILE *restrict stream) {
  size_t total = 0;
  WRITE_FIELD(total, stream, cg->keyspace.root);
  WRITE_FIELD(total, stream, cg->keyspace.symmetric_threats);
  WRITE_FIELD(total, stream, cg->tactics);
  WRITE_FIELD(total, stream, cg->num_moves);
  WRITE_ARRAY(total, stream, cg->moves, cg->num_moves);

  // The actual reader uses float values but we can write using a temporary fixed-point array
  size_t num_unique = 0;
  table_value *value_map = xcalloc(VALUE_MAP_SIZE, sizeof(table_value));

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
    value_id_t vid = (value_id_t)id;
    WRITE_FIELD(total, stream, vid);
  }

  for (size_t i = 0; i < VALUE_MAP_SIZE; ++i) {
    value v = table_value_to_value(value_map[i]);
    WRITE_FIELD(total, stream, v);
  }

  free(value_map);

  return total;
}

static void unbuffer_complete_graph_reader(complete_graph_reader *cgr) {
  char *map = cgr->buffer;
  state root;
  bool symmetric_threats;

  READ_FIELD(map, root);
  READ_FIELD(map, symmetric_threats);
  cgr->keyspace = create_tight_keyspace(&root, symmetric_threats);

  READ_FIELD(map, cgr->tactics);
  READ_FIELD(map, cgr->num_moves);

  cgr->moves = xmalloc(cgr->num_moves * sizeof(stones_t));
  memcpy(cgr->moves, map, cgr->num_moves * sizeof(stones_t));
  map += cgr->num_moves * sizeof(stones_t);

  MAP_ARRAY_FIELD(map, cgr->value_ids, cgr->keyspace.size);

  cgr->value_map = xmalloc(VALUE_MAP_SIZE * sizeof(value));
  memcpy(cgr->value_map, map, VALUE_MAP_SIZE * sizeof(value));
}

complete_graph_reader load_complete_graph_reader(const char *filename) {
  complete_graph_reader result;
  result.buffer = file_to_mmap(filename, &(result.sb), &(result.fd));
  unbuffer_complete_graph_reader(&result);
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
        child_value = apply_tactics(cgr->tactics, r, &child, get_complete_graph_reader_value_(cgr, &child, depth - 1));
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
  return (value){v.low + delta, v.high + delta};
}

value get_complete_graph_reader_value(const complete_graph_reader *cgr, const state *s) {
  return get_complete_graph_reader_value_(cgr, s, MAX_COMPENSATION_DEPTH);
}
