// Separate file for wide board symmetries

stones_t stones_mirror_v_w2(const stones_t stones) {
  return ((stones & NORTH_WALL_16) << V_SHIFT_16) | ((stones >> V_SHIFT_16) & NORTH_WALL_16);
}

stones_t stones_mirror_h_w4(const stones_t stones) {
    return (
      ((stones & VV0) << 3) |
      ((stones & VV1) << 1) |
      ((stones & VV2) >> 1) |
      ((stones & VV3) >> 3)
  );
}

stones_t stones_mirror_h_w6(stones_t stones) {
  stones = (
    ((stones & (VV0 | VV1 | VV2)) << 3) |
    ((stones & (VV3 | VV4 | VV5)) >> 3)
  );
  return (
    ((stones & (VV0 | VV3)) << 2) |
    (stones & (VV1 | VV4)) |
    ((stones & (VV2 | VV5)) >> 2)
  );
}

stones_t stones_mirror_h_w8(stones_t stones) {
  stones = (
    ((stones & (VV0 | VV1 | VV2 | VV3)) << 4) |
    ((stones & (VV4 | VV5 | VV6 | VV7)) >> 4)
  );
  stones = (
    ((stones & (VV0 | VV1 | VV4 | VV5)) << 2) |
    ((stones & (VV2 | VV3 | VV6 | VV7)) >> 2)
  );
  return (
    ((stones & (VV0 | VV2 | VV4 | VV6)) << 1) |
    ((stones & (VV1 | VV3 | VV5 | VV7)) >> 1)
  );
}

// @ @ @ @
// @ @ @ @
// wide = true

// Compression ratio ~ 3.86 (vs. ideal 4.0)
#define EVEN_EVEN_CORE_SIZE (1701)

size_t even_even_core_idx(stones_t black, stones_t white) {
  size_t result = black & 15;
  black >>= V_SHIFT_16;
  result |= (black & 15) << 4;

  result |= (white & 15) << 8;
  white >>= V_SHIFT_16;
  result |= (white & 15) << 12;

  return result;
}

void prepare_even_even_symmetry(symmetry *sym, stones_t visual_area) {
  sym->pulp_dots = dots(visual_area ^ (rectangle_16(4, 2) << sym->core_shift), &(sym->pulp_count));
  sym->core_idx = even_even_core_idx;
  size_t size = 1 << 16;
  sym->pulp_ops = malloc(size * sizeof(mirror_op_t));
  sym->core_map = malloc(size * sizeof(size_t));
  sym->core_m = 0;
  sym->black_core = malloc(EVEN_EVEN_CORE_SIZE * sizeof(stones_t));
  sym->white_core = malloc(EVEN_EVEN_CORE_SIZE * sizeof(stones_t));
  for (size_t idx = 0; idx < size; ++idx) {
    stones_t c = idx;
    stones_t black = c & 15;
    c >>= 4;
    black |= (c & 15) << V_SHIFT_16;
    c >>= 4;

    stones_t white = c & 15;
    c >>= 4;
    white |= (c & 15) << V_SHIFT_16;

    // Skip overlapping
    if (black & white) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }

    #ifdef CHECK_SYM_SANITY
      assert(sym->core_idx(black, white) == idx);
    #endif

    sym->pulp_ops[idx] = least_of_2(stones_mirror_v_w2, stones_mirror_h_w4, &black, &white);
    for (size_t i = 0; i < sym->core_m; ++i) {
      if (black == sym->black_core[i] && white == sym->white_core[i]) {
        sym->core_map[idx] = i;
        // continue outer;
        goto next_idx;
      }
    }
    sym->core_map[idx] = sym->core_m;
    sym->black_core[sym->core_m] = black;
    sym->white_core[sym->core_m] = white;
    sym->core_m++;

    // Label to break out of inner loops
    next_idx:
  }

  for (size_t i = 0; i < EVEN_EVEN_CORE_SIZE; ++i) {
    sym->black_core[i] <<= sym->core_shift;
    sym->white_core[i] <<= sym->core_shift;
  }
}
