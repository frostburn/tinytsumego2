#define _GNU_SOURCE // Expose declaration of tdestroy()
#include "tinytsumego2/dual_reader.h"
#include "jkiss/jkiss.h"
#include "tinytsumego2/keyspace.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/util.h"
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <search.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DUAL_READER_VERSION (5)

#define WRITE_FIELD(total, stream, field) (total += fwrite(&(field), sizeof(field), 1, (stream)) * sizeof(field))
#define WRITE_ARRAY(total, stream, ptr, count) (total += fwrite((ptr), sizeof(*(ptr)), (count), (stream)) * sizeof(*(ptr)))

#define READ_FIELD(map, field)             \
  do {                                     \
    memcpy(&(field), (map), sizeof(field)); \
    (map) += sizeof(field);                \
  } while (0)

#define MAP_ARRAY_FIELD(map, field, count)                         \
  do {                                                             \
    const size_t __count = (count);                               \
    (field) = (__typeof__(field))(map);                           \
    (map) += sizeof(*(field)) * __count;                          \
  } while (0)

static inline size_t frozen_tail_keys_size(size_t tail_size) { return tail_size / 2; }

size_t __to_compressed_key(const dual_graph_reader *dgr, const state *s) { return to_compressed_key(&(dgr->keyspace.compressed), s); }

size_t __to_symmetric_key(const dual_graph_reader *dgr, const state *s) { return to_symmetric_key(&(dgr->keyspace.symmetric), s); }

size_t write_dual_graph(const dual_graph *restrict dg, const frozen_hash_table *restrict fht, FILE *restrict stream) {
  int version = DUAL_READER_VERSION;
  size_t total = 0;
  WRITE_FIELD(total, stream, version);

  WRITE_FIELD(total, stream, dg->type);
  WRITE_FIELD(total, stream, dg->keyspace._.size);
  WRITE_FIELD(total, stream, dg->keyspace._.fast_size);
  WRITE_FIELD(total, stream, dg->keyspace._.prefix_m);
  WRITE_FIELD(total, stream, dg->keyspace._.root);

  const monotonic_compressor *comp = &(dg->keyspace._.compressor);
  WRITE_FIELD(total, stream, comp->num_checkpoints);
  WRITE_ARRAY(total, stream, comp->checkpoints, comp->num_checkpoints);
  WRITE_FIELD(total, stream, comp->uncompressed_size);
  WRITE_ARRAY(total, stream, comp->deltas, comp->uncompressed_size);
  WRITE_FIELD(total, stream, comp->size);
  WRITE_FIELD(total, stream, comp->factor);

  // Note: Specific keyspaces re-constructed on load

  WRITE_FIELD(total, stream, dg->num_moves);
  WRITE_ARRAY(total, stream, dg->moves, dg->num_moves);

  for (size_t i = 0; i < dg->keyspace._.size; ++i) {
    dual_table_value v = (dual_table_value){
        dg->plain_values[i],
        dg->forcing_values[i],
    };
    dual_table_value *tv = bsearch(&v, fht->bulk_map, fht->bulk_map_size, sizeof(dual_table_value), compare_dual_table_values);
    value_id_t vid = tv ? (value_id_t)(tv - fht->bulk_map) : VALUE_ID_SENTINEL;
    WRITE_FIELD(total, stream, vid);
  }

  WRITE_FIELD(total, stream, fht->bulk_map_size);
  WRITE_ARRAY(total, stream, fht->bulk_map, fht->bulk_map_size);

  WRITE_FIELD(total, stream, fht->tail_size);
  WRITE_ARRAY(total, stream, fht->tail_values, fht->tail_size);
  WRITE_ARRAY(total, stream, fht->tail_keys, frozen_tail_keys_size(fht->tail_size));

  return total;
}

void unbuffer_dual_graph_reader(dual_graph_reader *dgr) {
  char *map = dgr->buffer;

  int version = 0;
  READ_FIELD(map, version);

  if (version != DUAL_READER_VERSION) {
    fprintf(stderr, "Unknown dual graph version %d\n", version);
    exit(EXIT_FAILURE);
  }

  READ_FIELD(map, dgr->type);
  READ_FIELD(map, dgr->keyspace._.size);
  READ_FIELD(map, dgr->keyspace._.fast_size);
  READ_FIELD(map, dgr->keyspace._.prefix_m);
  READ_FIELD(map, dgr->keyspace._.root);

  monotonic_compressor *comp = &(dgr->keyspace._.compressor);

  READ_FIELD(map, comp->num_checkpoints);

  // Memory map aux data to save RAM
  MAP_ARRAY_FIELD(map, comp->checkpoints, comp->num_checkpoints);

  READ_FIELD(map, comp->uncompressed_size);

  MAP_ARRAY_FIELD(map, comp->deltas, comp->uncompressed_size);

  READ_FIELD(map, comp->size);

  READ_FIELD(map, comp->factor);

  if (dgr->type == COMPRESSED_KEYSPACE) {
    dgr->keyspace.compressed.keyspace = create_tight_keyspace(&(dgr->keyspace._.root), true);
    dgr->to_key = __to_compressed_key;
  } else if (dgr->type == SYMMETRIC_KEYSPACE) {
    dgr->keyspace.symmetric.symmetry = compute_symmetry(&(dgr->keyspace._.root));
    dgr->to_key = __to_symmetric_key;
  } else {
    // Support testing of mock keyspaces
    dgr->to_key = NULL;
  }

  READ_FIELD(map, dgr->num_moves);

  dgr->moves = xmalloc(dgr->num_moves * sizeof(stones_t));
  memcpy(dgr->moves, map, dgr->num_moves * sizeof(stones_t));
  map += dgr->num_moves * sizeof(stones_t);

  MAP_ARRAY_FIELD(map, dgr->value_table.bulk_ids, dgr->keyspace._.size);

  READ_FIELD(map, dgr->value_table.bulk_map_size);

  dgr->value_table.bulk_map = xmalloc(dgr->value_table.bulk_map_size * sizeof(dual_table_value));
  memcpy(dgr->value_table.bulk_map, map, dgr->value_table.bulk_map_size * sizeof(dual_table_value));
  map += sizeof(dual_table_value) * dgr->value_table.bulk_map_size;

  READ_FIELD(map, dgr->value_table.tail_size);

  MAP_ARRAY_FIELD(map, dgr->value_table.tail_values, dgr->value_table.tail_size);
  MAP_ARRAY_FIELD(map, dgr->value_table.tail_keys, frozen_tail_keys_size(dgr->value_table.tail_size));
}

dual_graph_reader load_dual_graph_reader(const char *filename) {
  dual_graph_reader result;
  char *map = file_to_mmap(filename, &(result.sb), &(result.fd));

  result.buffer = map;

  unbuffer_dual_graph_reader(&result);

  return result;
}

void unload_dual_graph_reader(dual_graph_reader *dgr) {
  if (dgr->type == COMPRESSED_KEYSPACE) {
    free_tight_keyspace(&(dgr->keyspace.compressed.keyspace));
  } else if (dgr->type == SYMMETRIC_KEYSPACE) {
    free_symmetry(&(dgr->keyspace.symmetric.symmetry));
  } else {
    printf("Unloading mock keyspace\n");
  }

  free(dgr->moves);
  dgr->num_moves = 0;
  dgr->moves = NULL;

  free(dgr->value_table.bulk_map);
  dgr->value_table.bulk_map_size = 0;
  dgr->value_table.bulk_map = NULL;

  if (dgr->fd >= 0) {
    munmap(dgr->buffer, dgr->sb.st_size);
    close(dgr->fd);

    dgr->buffer = NULL;
    dgr->fd = -1;

    dgr->keyspace._.compressor.checkpoints = NULL;
    dgr->keyspace._.compressor.deltas = NULL;
    dgr->value_table.bulk_ids = NULL;
  }
}

dual_value get_dual_graph_reader_value_(const dual_graph_reader *dgr, const state *s, int depth) {
  if (!depth) {
    return (dual_value){{-INFINITY, INFINITY}, {-INFINITY, INFINITY}};
  }
  if (target_capturable(s)) {
    float sc = take_target_score(s);
    dual_value v = (dual_value){{sc, sc}, {sc, sc}};
    if (!s->passes && !s->button) {
      state child = *s;
      const move_result r = make_move(&child, pass());
      dual_value child_value = get_dual_graph_reader_value_(dgr, &child, depth - 1);
      child_value.plain = apply_tactics(NONE, r, &child, child_value.plain);
      child_value.forcing = apply_tactics(FORCING, r, &child, child_value.forcing);
      v.plain.low = fmax(v.plain.low, child_value.plain.high);
      v.plain.high = fmax(v.plain.high, child_value.plain.low);
      v.forcing.low = fmax(v.forcing.low, child_value.forcing.high);
      v.forcing.high = fmax(v.forcing.high, child_value.forcing.low);
    }
    return v;
  }
  if (s->passes || s->ko || target_in_atari(s)) {
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
        assert(r != TAKE_TARGET);
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
    key = dgr->to_key(dgr, &c);
    delta = -2 * BUTTON_BONUS;
  } else {
    key = dgr->to_key(dgr, s);
  }

  dual_table_value tv = get_frozen_hash_value(&(dgr->value_table), key);
  return (dual_value){
      {
          score_q7_to_float(tv.plain.low) + delta,
          score_q7_to_float(tv.plain.high) + delta,
      },
      {
          score_q7_to_float(tv.forcing.low) + delta,
          score_q7_to_float(tv.forcing.high) + delta,
      },
  };
}

dual_value get_dual_graph_reader_value(const dual_graph_reader *dgr, const state *s) {
  return get_dual_graph_reader_value_(dgr, s, MAX_COMPENSATION_DEPTH);
}

dual_graph_reader *allocate_dual_graph_reader(const char *filename) {
  dual_graph_reader *result = xmalloc(sizeof(dual_graph_reader));
  *result = load_dual_graph_reader(filename);
  return result;
}

stones_t *dual_graph_reader_python_stuff(dual_graph_reader *dgr, state *root, int *num_moves) {
  *root = dgr->keyspace._.root;
  *num_moves = dgr->num_moves;
  return dgr->moves;
}

state strip_aesthetics(const dual_graph_reader *dgr, const state *s) {
  state ss = *s;
  ss.button = abs(ss.button);
  if (dgr->type == COMPRESSED_KEYSPACE) {
    size_t key = to_tight_key_fast(&(dgr->keyspace.compressed.keyspace), &ss);
    ss = from_tight_key_fast(&(dgr->keyspace.compressed.keyspace), key);
  } else {
    // Symmetric states don't have aesthetics yet
    return *s;
  }
  ss.ko = s->ko;
  ss.passes = s->passes;
  ss.button = s->button;
  stones_t swaps = ss.external ^ s->external;
  ss.immortal ^= swaps;
  ss.external ^= swaps;
  ss.logical_area ^= swaps;

  return ss;
}

move_info *dual_graph_reader_move_infos(const dual_graph_reader *dgr, const state *s, int *num_move_infos) {
  move_info *result = xmalloc(dgr->num_moves * sizeof(move_info));
  *num_move_infos = 0;

  state parent = strip_aesthetics(dgr, s);

  dual_value v = get_dual_graph_reader_value(dgr, &parent);

  float lows_high = -INFINITY;
  float highs_low = -INFINITY;
  dual_value *child_values = xmalloc(dgr->num_moves * sizeof(dual_value));
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
    result[(*num_move_infos)++] = (move_info){
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

int compare_dual_table_values(const void *a_, const void *b_) {
  dual_table_value *a = (dual_table_value *)a_;
  dual_table_value *b = (dual_table_value *)b_;

  if (a->plain.low < b->plain.low) {
    return -1;
  }
  if (a->plain.low > b->plain.low) {
    return 1;
  }

  if (a->plain.high < b->plain.high) {
    return -1;
  }
  if (a->plain.high > b->plain.high) {
    return 1;
  }

  if (a->forcing.low < b->forcing.low) {
    return -1;
  }
  if (a->forcing.low > b->forcing.low) {
    return 1;
  }

  if (a->forcing.high < b->forcing.high) {
    return -1;
  }
  if (a->forcing.high > b->forcing.high) {
    return 1;
  }

  return 0;
}

typedef struct tree_value {
  dual_table_value value;
  size_t count;
} tree_value;

frozen_hash_table prepare_frozen_hash(const dual_graph *dg, size_t *num_unique) {
  void *root = NULL;
  dual_table_value *v;
  dual_table_value **tv;
  *num_unique = 0;

  for (size_t i = 0; i < dg->keyspace._.size; ++i) {
    // Allocate value and count
    v = xmalloc(sizeof(tree_value));
    // Abuse struct overlap
    v->plain.low = dg->plain_values[i].low;
    v->plain.high = dg->plain_values[i].high;
    v->forcing.low = dg->forcing_values[i].low;
    v->forcing.high = dg->forcing_values[i].high;
    tv = tsearch(v, &root, compare_dual_table_values);
    if (!tv) {
      exit(EXIT_FAILURE);
    }
    if (*tv == v) {
      (*num_unique)++;
      ((tree_value *)v)->count = 1;
    } else {
      free(v);

      v = *tv;
      ((tree_value *)v)->count++;
    }
  }

  dual_table_value *value_map = xmalloc((*num_unique) * sizeof(dual_table_value));

  size_t i = 0;
  void action(const void *nodep, VISIT which, int) {
    if (which == postorder || which == leaf) {
      value_map[i++] = (*(tree_value **)nodep)->value;
    }
  }
  twalk(root, action);

  if ((*num_unique) <= VALUE_MAP_SIZE - 1) {
    tdestroy(root, free);

    return (frozen_hash_table){*num_unique, value_map, NULL, 0, NULL, NULL};
  }

  int cmp(const void *a_, const void *b_) {
    tree_value *a = *(tree_value **)tfind(a_, &root, compare_dual_table_values);
    tree_value *b = *(tree_value **)tfind(b_, &root, compare_dual_table_values);

    // Sort most common to front
    if (a->count > b->count) {
      return -1;
    }
    if (a->count < b->count) {
      return 1;
    }
    return 0;
  }
  qsort(value_map, *num_unique, sizeof(dual_table_value), cmp);

  const size_t n = VALUE_MAP_SIZE - 1;

  size_t tail_size = 0;
  for (size_t i = n; i < *num_unique; ++i) {
    tail_size += (*(tree_value **)tfind(value_map + i, &root, compare_dual_table_values))->count;
  }

  tdestroy(root, free);

  value_map = xrealloc(value_map, n * sizeof(dual_table_value));
  qsort(value_map, n, sizeof(dual_table_value), compare_dual_table_values);

  size_t *tail_keys = xmalloc(frozen_tail_keys_size(tail_size) * sizeof(size_t));
  dual_table_value *tail_values = xmalloc(tail_size * sizeof(dual_table_value));

  v = xmalloc(sizeof(dual_table_value));
  size_t j = 0;
  for (size_t i = 0; i < dg->keyspace._.size; ++i) {
    v->plain.low = dg->plain_values[i].low;
    v->plain.high = dg->plain_values[i].high;
    v->forcing.low = dg->forcing_values[i].low;
    v->forcing.high = dg->forcing_values[i].high;
    if (!bsearch(v, value_map, n, sizeof(dual_table_value), compare_dual_table_values)) {
      if (j % 2 == 1) {
        tail_keys[j / 2] = i;
      }
      tail_values[j++] = *v;
    }
  }
  assert(j == tail_size);
  free(v);

  return (frozen_hash_table){n, value_map, NULL, tail_size, tail_values, tail_keys};
}

dual_table_value get_frozen_hash_value(const frozen_hash_table *fht, size_t key) {
  value_id_t vid = fht->bulk_ids[key];
  if (vid == VALUE_ID_SENTINEL) {
    const size_t tail_keys_size = frozen_tail_keys_size(fht->tail_size);
    size_t lo = 0;
    size_t hi = tail_keys_size;
    while (lo < hi) {
      size_t mid = lo + (hi - lo) / 2;
      size_t mid_key = fht->tail_keys[mid];
      if (mid_key < key) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    size_t i = 2 * lo;
    if (lo < tail_keys_size && fht->tail_keys[lo] == key) {
      i += 1;
    }
    assert(i < fht->tail_size);
    return fht->tail_values[i];
  }
  return fht->bulk_map[vid];
}
