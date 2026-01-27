//#included in base.c

// Make sure that the eyespace fits the table and that the target lining is to the east and south. Assumes s->wide == false
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

size_t to_corner_tablebase_key(const state *s) {
  if (s->wide) {
    return INVALID_KEY;
  }

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
  s.target = rectangle(TABLE_WIDTH + 1, TABLE_HEIGHT + 1) ^ s.logical_area;
  s.player |= s.target;
  s.target |= flood(s.target, s.player);
  s.logical_area &= ~s.target;
  s.visual_area = s.logical_area | s.target;
  return s;
}
