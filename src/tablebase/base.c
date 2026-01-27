#include <math.h>
#include "tinytsumego2/tablebase.h"

bool can_be_tabulated(const state *s) {
  // Ko not indexed
  if (s->ko) {
    return false;
  }

  // Some eyespace must exist
  stones_t eyespace = s->logical_area ^ s->external;
  if (!eyespace) {
    return false;
  }

  // Target must belong to the player
  if (!(s->target & s->player)) {
    return false;
  }

  // Must have a single clump of target stones
  if (!(s->wide ? is_contiguous_16(s->target) : is_contiguous(s->target))) {
    return false;
  }

  // Eyespace must be lined with target stones
  stones_t libs = s->wide ? liberties_16(eyespace, s->visual_area) : liberties(eyespace, s->visual_area);
  if (libs & ~s->target) {
    return false;
  }

  // External liberties must attached to the target
  libs = s->wide ? liberties_16(s->target, s->external) : liberties(s->target, s->external);
  if (libs != s->external) {
    return false;
  }

  // External liberties must belong to the capturing player
  if (s->external & s->player) {
    return false;
  }

  return true;
}

#include "corner.c"
#include "edge.c"

value get_tablebase_value(const tablebase *tb, const state *s) {
  if (s->passes != 0) {
    return (value) {NAN, NAN};
  }
  bool opponent_targetted = s->opponent & s->target;
  if (opponent_targetted) {
    if (s->player & s->target) {
      return (value) {NAN, NAN};
    }
  } else if (!(s->player & s->target)) {
    return (value) {NAN, NAN};
  }
  int num_external = popcount(s->external);
  for (size_t i = 0; i < tb->num_tables; ++i) {
    if (
      tb->tables[i].button == abs(s->button) &&
      tb->tables[i].ko_threats == s->ko_threats &&
      tb->tables[i].num_external == num_external &&
      tb->tables[i].opponent_targetted == opponent_targetted
    ) {
      size_t key;
      if (opponent_targetted) {
        state c = *s;
        stones_t temp = c.player;
        c.player = c.opponent;
        c.opponent = temp;
        switch (tb->tables[i].type) {
          case CORNER:
            key = to_corner_tablebase_key(&c);
            break;
          case EDGE:
            key = to_edge_tablebase_key(&c);
            break;
          default:
            fprintf(stderr, "Center not implemented");
            exit(EXIT_FAILURE);
        }
      } else {
        switch (tb->tables[i].type) {
          case CORNER:
            key = to_corner_tablebase_key(s);
            break;
          case EDGE:
            key = to_edge_tablebase_key(s);
            break;
          default:
            fprintf(stderr, "Center not implemented");
            exit(EXIT_FAILURE);
        }
      }
      if (key == INVALID_KEY) {
        continue;
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
      if (tb->tables[i].button != s->button) {
        // Compensate button ownership even for target captures
        result.low -= 2 * BUTTON_BONUS;
        result.high -= 2 * BUTTON_BONUS;
      }
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
  total += fwrite(&(tt->opponent_targetted), sizeof(bool), 1, stream);
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

#include "util.c"

tsumego_table read_tsumego_table(FILE *restrict stream) {
  tsumego_table result;
  read_one(&(result.type), sizeof(table_type), stream);
  read_one(&(result.button), sizeof(int), stream);
  read_one(&(result.ko_threats), sizeof(int), stream);
  read_one(&(result.num_external), sizeof(int), stream);
  read_one(&(result.opponent_targetted), sizeof(bool), stream);
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
