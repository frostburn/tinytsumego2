#include <math.h>
#include "tinytsumego2/tablebase.h"

// Make sure that the eyespace fits the table and that the target lining is to the east and south
bool fits_corner(state *s) {
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

// TODO: Something to pre-orient the root state at the upper left corner

size_t to_corner_tablebase_key(state *s, float *delta) {
  *delta = NAN;
  // Ko not indexed
  if (s->ko) {
    return INVALID_KEY;
  }
  // Target must belong to the player
  if (!(s->target & s->player)) {
    return INVALID_KEY;
  }
  // Must have a single clump of target stones
  if (!s->target || !is_contiguous(s->target)) {
    return INVALID_KEY;
  }

  // Eyespace must be lined with target stones
  stones_t eyespace = s->logical_area ^ s->external;
  if (liberties(eyespace, s->visual_area) & ~s->target) {
    return INVALID_KEY;
  }

  // External liberties must attached to the target
  if (liberties(s->target, s->external) != s->external) {
    return INVALID_KEY;
  }

  // External liberties must belong to the capturing player
  if (s->external & s->player) {
    return INVALID_KEY;
  }

  // Must actually fit the tablebase
  if (!fits_corner(s)) {
    return INVALID_KEY;
  }

  // Check for gaps in the goban
  eyespace = s->logical_area ^ s->external;
  if (cross(eyespace) & ~s->visual_area) {
    return INVALID_KEY;
  }

  // TODO: At least flip diagonally if it could help

  // TODO: Block conversion
  // TERNARY[(s->player & TABLE_MASK) | ((s->opponent & TABLE_MASK) << TABLE_WIDTH)] + 3**TABLE_WIDTH * TERNARY[...];

  stones_t outside = ~rectangle(TABLE_WIDTH, TABLE_HEIGHT);
  state c = *s;
  c.visual_area &= outside;
  c.player &= outside;
  c.opponent &= outside;
  c.external &= outside;

  // TODO: Figure out opponent liberties radiating from the rectangle
  *delta = -chinese_liberty_score(&c);

  size_t key = 0;
  for (int y = TABLE_HEIGHT - 1; y >= 0; --y) {
    stones_t p = 1ULL << (TABLE_WIDTH - 1 + y * V_SHIFT);
    for (int x = TABLE_WIDTH - 1; x >= 0; --x) {
      key *= 3;
      if (s->player & p) {
        key += 1;
      } else if (!(eyespace & p)) {
        // Voids become player target stones
        key += 1;
        if (s->opponent & p) {
          *delta -= 2;
        } else {
          *delta -= 1;
        }
      } else if (s->opponent & p) {
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
