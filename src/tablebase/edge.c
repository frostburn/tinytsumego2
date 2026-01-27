//#included in base.c

// Make sure that the eyespace fits the table and that the target lining is to the east, west and south. Assumes s->wide == true
bool fits_edge(const state *s) {
  stones_t eyespace = s->logical_area ^ s->external;
  int x = offset_h_16(eyespace);
  int width = width_of_16(eyespace);
  int height = height_of_16(eyespace);
  return (
    (width - x) <= TABLE_WIDTH &&
    height <= TABLE_HEIGHT &&
    (s->target & (WEST_WALL_16 << x)) &&
    (s->target & (WEST_WALL_16 << width)) &&
    (s->target & (NORTH_WALL_16 << (height * V_SHIFT_16)))
  );
}

size_t to_edge_tablebase_key(const state *s) {
  if (!s->wide) {
    return INVALID_KEY;
  }

  state c = *s;
  snap(&c);

  // Must actually fit the tablebase
  if (!fits_edge(&c)) {
    mirror_v(&c);
    snap(&c);
    if (!fits_edge(&c)) {
      return INVALID_KEY;
    }
  }

  // Check for gaps in the goban
  stones_t eyespace = c.logical_area ^ c.external;
  if (cross_16(eyespace) & ~c.visual_area) {
    return INVALID_KEY;
  }

  int offset_x = offset_h_16(eyespace);

  // TODO: Block conversion

  size_t key = 0;
  for (int y = TABLE_HEIGHT - 1; y >= 0; --y) {
    stones_t p = 1ULL << (offset_x + TABLE_WIDTH - 1 + y * V_SHIFT_16);
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

state from_edge_tablebase_key(size_t key) {
  state s = {0};
  s.wide = true;
  stones_t p;
  for (int y = 0; y < TABLE_HEIGHT; ++y) {
    p = 1ULL << (H_SHIFT_16 + y * V_SHIFT_16);
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
  s.logical_area = rectangle_16(TABLE_WIDTH, TABLE_HEIGHT) << H_SHIFT_16;
  s.target = rectangle_16(TABLE_WIDTH + 2, TABLE_HEIGHT + 1) ^ s.logical_area;
  s.player |= s.target;
  s.target |= flood_16(s.target, s.player);
  s.logical_area &= ~s.target;
  s.visual_area = s.logical_area | s.target;
  return s;
}
