#include <math.h>
#include "tinytsumego2/tablebase.h"

// Make sure that the eyespace fits the table and that the target lining is to the east and south
bool fits_corner(const state *s) {
  stones_t eyespace = s->logical_area ^ s->external;
  int width = width_of(eyespace);
  int height = height_of(eyespace);
  return (
    width <= TABLE_WIDTH &&
    height <= TABLE_HEIGHT &&
    (s->target & (WEST_WALL << width)) &&
    (s->target & (NORTH_WALL << (height * V_SHIFT)))
  );
}

bool can_be_tabulated(const state *s) {
  // Ko not indexed
  if (s->ko) {
    return false;
  }

  // Some eyespace must exist
  if (!s->logical_area) {
    return false;
  }

  // Target must belong to the player
  if (!(s->target & s->player)) {
    return false;
  }

  // Must have a single clump of target stones
  if (!s->target || !is_contiguous(s->target)) {
    return false;
  }

  // Eyespace must be lined with target stones
  stones_t eyespace = s->logical_area ^ s->external;
  if (liberties(eyespace, s->visual_area) & ~s->target) {
    return false;
  }

  // External liberties must attached to the target
  if (liberties(s->target, s->external) != s->external) {
    return false;
  }

  // External liberties must belong to the capturing player
  if (s->external & s->player) {
    return false;
  }

  return true;
}

size_t to_corner_tablebase_key(const state *s) {
  if (!can_be_tabulated(s)) {
    return INVALID_KEY;
  }

  state c = *s;
  snap(&c);

  // Must actually fit the tablebase
  if (!fits_corner(&c)) {
    mirror_v(&c);
    snap(&c);
    if (!fits_corner(&c)) {
      mirror_h(&c);
      snap(&c);
      if (!fits_corner(&c)) {
        mirror_v(&c);
        snap(&c);
        if (!fits_corner(&c)) {
          // TODO: Flip diagonally if table width != height
          return INVALID_KEY;
        }
      }
    }
  }

  // Check for gaps in the goban
  stones_t eyespace = c.logical_area ^ c.external;
  if (cross(eyespace) & ~c.visual_area) {
    return INVALID_KEY;
  }

  // TODO: Block conversion
  // TERNARY[(c.player & TABLE_MASK) | ((c.opponent & TABLE_MASK) << TABLE_WIDTH)] + 3**TABLE_WIDTH * TERNARY[...];

  size_t key = 0;
  for (int y = TABLE_HEIGHT - 1; y >= 0; --y) {
    stones_t p = 1ULL << (TABLE_WIDTH - 1 + y * V_SHIFT);
    for (int x = TABLE_WIDTH - 1; x >= 0; --x) {
      key *= 3;
      if (c.player & p) {
        key += 1;
      } else if (!(eyespace & p)) {
        // Voids become player target stones
        key += 1;
      } else if (c.opponent & p) {
        key += 2;
      }
      p >>= 1;
    }
  }
  return key;
}

state from_corner_tablebase_key(size_t key) {
  state s = {0};
  stones_t p;
  for (int y = 0; y < TABLE_HEIGHT; ++y) {
    p = 1ULL << (y * V_SHIFT);
    for (int x = 0; x < TABLE_WIDTH; ++x) {
      switch (key % 3) {
        case 1:
          s.player |= p;
          break;
        case 2:
          s.opponent |= p;
          break;
      }
      p <<= 1;
      key /= 3;
    }
  }
  s.logical_area = rectangle(TABLE_WIDTH, TABLE_HEIGHT);
  s.target |= rectangle(TABLE_WIDTH + 1, TABLE_HEIGHT + 1) ^ s.logical_area;
  s.player |= s.target;
  s.target |= flood(s.target, s.player);
  s.logical_area &= ~s.target;
  s.visual_area = s.logical_area | s.target;
  return s;
}

value get_tablebase_value(const tablebase *tb, const state *s) {
  if (s->passes != 0) {
    return (value) {NAN, NAN};
  }
  int num_external = popcount(s->external);
  for (size_t i = 0; i < tb->num_tables; ++i) {
    if (
      tb->tables[i].button == s->button &&
      tb->tables[i].ko_threats == s->ko_threats &&
      tb->tables[i].num_external == num_external
    ) {
      size_t key = to_corner_tablebase_key(s);
      if (key == INVALID_KEY) {
        return (value) {NAN, NAN};
      }
      table_value v = tb->tables[i].values[key];
      if (v.low == INVALID_SCORE_Q7) {
        fprintf(stderr, "Unexpected tablebase miss\n");
        return (value) {NAN, NAN};
      }

      value result = (value) {
        score_q7_to_float(v.low),
        score_q7_to_float(v.high)
      };

      // Only delta-values are tabulated. Add in a baseline unless the target can be captured
      float baseline = compensated_liberty_score(s);
      if (abs(v.low) < BIG_SCORE_Q7) {
        result.low += baseline;
      }
      if (abs(v.high) < BIG_SCORE_Q7) {
        result.high += baseline;
      }
      return result;
    }
  }
  return (value) {NAN, NAN};
}

size_t write_tsumego_table(const tsumego_table *restrict tt, FILE *restrict stream) {
  size_t total = fwrite(&(tt->type), sizeof(table_type), 1, stream);
  total += fwrite(&(tt->button), sizeof(int), 1, stream);
  total += fwrite(&(tt->ko_threats), sizeof(int), 1, stream);
  total += fwrite(&(tt->num_external), sizeof(int), 1, stream);
  total += fwrite(tt->values, sizeof(table_value), TABLEBASE_SIZE, stream);
  return total;
}

size_t write_tablebase(const tablebase *restrict tb, FILE *restrict stream) {
  size_t total = fwrite(&(tb->num_tables), sizeof(size_t), 1, stream);
  for (size_t i = 0; i < tb->num_tables; ++i) {
    total += write_tsumego_table(tb->tables + i, stream);
  }
  return total;
}

void read_one(void *restrict ptr, size_t size, FILE *restrict stream) {
  if (!fread(ptr, size, 1, stream)) {
    fprintf(stderr, "Failed to read a single item\n");
    exit(EXIT_FAILURE);
  }
}

void read_many(void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream) {
  if (!fread(ptr, size, nitems, stream)) {
    fprintf(stderr, "Failed to read %zu item\n", nitems);
    exit(EXIT_FAILURE);
  }
}

tsumego_table read_tsumego_table(FILE *restrict stream) {
  tsumego_table result;
  read_one(&(result.type), sizeof(table_type), stream);
  read_one(&(result.button), sizeof(int), stream);
  read_one(&(result.ko_threats), sizeof(int), stream);
  read_one(&(result.num_external), sizeof(int), stream);
  result.values = malloc(sizeof(table_value) * TABLEBASE_SIZE);
  read_many(result.values, sizeof(table_value), TABLEBASE_SIZE, stream);
  return result;
}

tablebase read_tablebase(FILE *restrict stream) {
  tablebase result;
  read_one(&(result.num_tables), sizeof(size_t), stream);
  result.tables = malloc(sizeof(tsumego_table) * result.num_tables);
  for (size_t i = 0; i < result.num_tables; ++i) {
    result.tables[i] = read_tsumego_table(stream);
  }
  return result;
}

void free_tablebase(tablebase *tb) {
  for (size_t i = 0; i < tb->num_tables; ++i) {
    free(tb->tables[i].values);
  }
  tb->num_tables = 0;
  free(tb->tables);
  tb->tables = NULL;
}
