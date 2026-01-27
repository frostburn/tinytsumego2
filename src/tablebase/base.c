#include <math.h>
#include "tinytsumego2/tablebase.h"

#include "corner.c"
#include "edge.c"

// Check if the state could be manipulated to determine that the target stones are dead
bool can_be_pseudo_tabulated(const state *s) {
  // Ko not indexed
  if (s->ko) {
    return false;
  }

  // Target capture must be possible
  if (s->target & s->immortal) {
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

  // External liberties must belong to the capturing player
  if (s->external & s->player) {
    return false;
  }

  return true;
}

// Check if the state resembles a tabulated state so that its value can be queried
bool can_be_hard_tabulated(const state *s) {
  stones_t eyespace = s->logical_area ^ s->external;

  // Eyespace must be lined with target stones
  stones_t libs = s->wide ? liberties_16(eyespace, s->visual_area) : liberties(eyespace, s->visual_area);
  if (libs & ~s->target) {
    return false;
  }

  // External liberties must be attached to the target
  libs = s->wide ? liberties_16(s->target, s->external) : liberties(s->target, s->external);
  if (libs != s->external) {
    return false;
  }

  return true;
}

// Modify state so that it fits in the tablebase without changing its status
bool manipulate_state(state *s) {
  stones_t target_libs = s->wide ? liberties_16(s->target, s->logical_area) : liberties(s->target, s->logical_area);

  int num_regions;
  stones_t *regions = s->wide ? chains_16(s->logical_area, &num_regions) : chains(s->logical_area, &num_regions);

  bool has_eyespace = false;

  for (int i = 0; i < num_regions; ++i) {
    // External liberties are fine
    if (!(regions[i] & ~s->external)) {
      continue;
    }
    stones_t libs = s->wide ? liberties_16(regions[i], s->visual_area) : liberties(regions[i], s->visual_area);
    if (!(libs & ~s->target)) {
      // Only one eyespace allowed
      if (has_eyespace) {
        free(regions);
        return false;
      }
      has_eyespace = true;
      continue;
    }
    // Convert physical liberties to logical liberties
    if (!(regions[i] & ~target_libs)) {
      stones_t filled = regions[i] & s->opponent;
      if (filled) {
        free(regions);
        return false;
      }
      // s->immortal |= filled;  // TODO: Check if filled liberties could be included
      s->external |= regions[i] ^ filled;
      s->opponent |= regions[i];
      continue;
    }
    // Bad region detected
    free(regions);
    return false;
  }
  free(regions);
  return has_eyespace;
}

size_t to_tablebase_key(table_type type, const state *s) {
  switch(type) {
    case CORNER:
      return to_corner_tablebase_key(s);
    case EDGE:
      return to_edge_tablebase_key(s);
    default:
      fprintf(stderr, "Center not implemented");
      exit(EXIT_FAILURE);
  }
  // Unreachable
  return INVALID_KEY;
}

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
  bool target_capture_only = false;
  state c = *s;
  if (opponent_targetted) {
    stones_t temp = c.player;
    c.player = c.opponent;
    c.opponent = temp;
  }
  if (can_be_pseudo_tabulated(&c)) {
    if (!can_be_hard_tabulated(&c)) {
      if (!manipulate_state(&c)) {
        return (value) {NAN, NAN};
      }
      if (!can_be_hard_tabulated(&c)) {
        return (value) {NAN, NAN};
      }
      target_capture_only = true;
    }
  } else {
    return (value) {NAN, NAN};
  }
  // Baseline must use the original state
  float baseline = compensated_liberty_score(s);
  // External liberties may have been manipulated
  int num_external = popcount(c.external);
  for (size_t i = 0; i < tb->num_tables; ++i) {
    if (
      tb->tables[i].button == abs(c.button) &&
      tb->tables[i].ko_threats == c.ko_threats &&
      tb->tables[i].num_external == num_external &&
      tb->tables[i].opponent_targetted == opponent_targetted
    ) {
      size_t key;
      key = to_tablebase_key(tb->tables[i].type, &c);
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

      if (target_capture_only) {
        if (abs(v.low) < BIG_SCORE_Q7 || abs(v.high) < BIG_SCORE_Q7) {
          continue;
        }
        return result;
      }

      // Only delta-values are tabulated. Add in a baseline unless the target can be captured
      if (tb->tables[i].button != c.button) {
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
