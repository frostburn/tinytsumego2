// Separate file for wide board symmetries

stones_t stones_mirror_v_w2(const stones_t stones) {
  return ((stones & NORTH_WALL_16) << V_SHIFT_16) | ((stones >> V_SHIFT_16) & NORTH_WALL_16);
}

stones_t stones_mirror_h_w3(const stones_t stones) { return (((stones & VV0) << 2) | (stones & VV1) | ((stones & VV2) >> 2)); }

stones_t stones_mirror_h_w4(const stones_t stones) {
  return (((stones & VV0) << 3) | ((stones & VV1) << 1) | ((stones & VV2) >> 1) | ((stones & VV3) >> 3));
}

stones_t stones_mirror_h_w5(const stones_t stones) {
  return (((stones & VV0) << 4) | ((stones & VV1) << 2) | (stones & VV2) | ((stones & VV3) >> 2) | ((stones & VV4) >> 4));
}

stones_t stones_mirror_h_w6(stones_t stones) {
  stones = (((stones & (VV0 | VV1 | VV2)) << 3) | ((stones & (VV3 | VV4 | VV5)) >> 3));
  return (((stones & (VV0 | VV3)) << 2) | (stones & (VV1 | VV4)) | ((stones & (VV2 | VV5)) >> 2));
}

stones_t stones_mirror_h_w7(stones_t stones) {
  stones = (((stones >> 4) & (VV0 | VV1 | VV2)) | (stones & VV3) | ((stones & (VV0 | VV1 | VV2)) << 4));
  return (((stones >> 2) & (VV0 | VV4)) | (stones & (VV1 | VV3 | VV5)) | ((stones & (VV0 | VV4)) << 2));
}

stones_t stones_mirror_h_w8(stones_t stones) {
  stones = (((stones & (VV0 | VV1 | VV2 | VV3)) << 4) | ((stones & (VV4 | VV5 | VV6 | VV7)) >> 4));
  stones = (((stones & (VV0 | VV1 | VV4 | VV5)) << 2) | ((stones & (VV2 | VV3 | VV6 | VV7)) >> 2));
  return (((stones & (VV0 | VV2 | VV4 | VV6)) << 1) | ((stones & (VV1 | VV3 | VV5 | VV7)) >> 1));
}

#define VVB3 (VV0 | VV1 | VV2)
#define VVBC3 (VV3 | VV4 | VV5)
#define VVE9 (VV0 | VV3 | VV6)
#define VVC9 (VV1 | VV4 | VV7)

stones_t stones_mirror_h_w9(stones_t stones) {
  stones = ((stones >> 6) & VVB3) | ((stones & VVB3) << 6) | (stones & VVBC3);
  return ((stones >> 2) & VVE9) | ((stones & VVE9) << 2) | (stones & VVC9);
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
  sym->pulp_ops = xmalloc(size * sizeof(mirror_op_t));
  sym->core_map = xmalloc(size * sizeof(size_t));
  sym->core_m = 0;
  sym->black_core = xmalloc(EVEN_EVEN_CORE_SIZE * sizeof(stones_t));
  sym->white_core = xmalloc(EVEN_EVEN_CORE_SIZE * sizeof(stones_t));
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

// @ @ @ @ @
// @ @ @ @ @
// wide = true, height = 2

// Compression ratio ~ 4.59 (vs. naïve ideal 4.0)
#define ODD_TWO_CORE_SIZE (12857)

size_t odd_two_core_idx(stones_t black, stones_t white) {
  size_t result = black & 31;
  black >>= V_SHIFT_16;
  result |= (black & 31) << 5;

  result |= (white & 31) << 10;
  white >>= V_SHIFT_16;
  result |= (white & 31) << 15;

  return result;
}

void prepare_odd_two_symmetry(symmetry *sym, stones_t visual_area) {
  sym->pulp_dots = dots(visual_area ^ (rectangle_16(5, 2) << sym->core_shift), &(sym->pulp_count));
  sym->core_idx = odd_two_core_idx;
  size_t size = 1 << 20;
  sym->pulp_ops = xmalloc(size * sizeof(mirror_op_t));
  sym->core_map = xmalloc(size * sizeof(size_t));
  sym->core_m = 0;
  sym->black_core = xmalloc(ODD_TWO_CORE_SIZE * sizeof(stones_t));
  sym->white_core = xmalloc(ODD_TWO_CORE_SIZE * sizeof(stones_t));
  for (size_t idx = 0; idx < size; ++idx) {
    stones_t c = idx;
    stones_t black = c & 31;
    c >>= 5;
    black |= (c & 31) << V_SHIFT_16;
    c >>= 5;

    stones_t white = c & 31;
    c >>= 5;
    white |= (c & 31) << V_SHIFT_16;

    // Skip overlapping
    if (black & white) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }

    // Skip illegal
    c = rectangle_16(7, 2);
    int num_chains = 0;
    stones_t *cs = chains_16(black << 1, &num_chains);
    for (int i = 0; i < num_chains; ++i) {
      if (!liberties_16(cs[i], c & ~(white << 1))) {
        sym->pulp_ops[idx] = UCHAR_MAX;
        sym->core_map[idx] = SIZE_MAX;
        free(cs);
        goto next_idx;
      }
    }
    free(cs);

    cs = chains_16(white << 1, &num_chains);
    for (int i = 0; i < num_chains; ++i) {
      if (!liberties_16(cs[i], c & ~(black << 1))) {
        sym->pulp_ops[idx] = UCHAR_MAX;
        sym->core_map[idx] = SIZE_MAX;
        free(cs);
        goto next_idx;
      }
    }
    free(cs);

#ifdef CHECK_SYM_SANITY
    assert(sym->core_idx(black, white) == idx);
#endif

    sym->pulp_ops[idx] = least_of_2(stones_mirror_v_w2, stones_mirror_h_w5, &black, &white);
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

  for (size_t i = 0; i < ODD_TWO_CORE_SIZE; ++i) {
    sym->black_core[i] <<= sym->core_shift;
    sym->white_core[i] <<= sym->core_shift;
  }
}

// @ @ @
// @ @ @
// @ @ @
// @ @ @
// wide = true, (height = 4)

// Compression ratio ~ 4.22 (vs. naïve ideal 4.0)
#define ODD_FOUR_CORE_SIZE (125955)

size_t odd_four_core_idx(stones_t black, stones_t white) {
  size_t result = black & 7;
  black >>= V_SHIFT_16;
  result |= (black & 7) << 3;
  black >>= V_SHIFT_16;
  result |= (black & 7) << 6;
  black >>= V_SHIFT_16;
  result |= (black & 7) << 9;

  result |= (white & 7) << 12;
  white >>= V_SHIFT_16;
  result |= (white & 7) << 15;
  white >>= V_SHIFT_16;
  result |= (white & 7) << 18;
  white >>= V_SHIFT_16;
  result |= (white & 7) << 21;

  return result;
}

void prepare_odd_four_symmetry(symmetry *sym, stones_t visual_area) {
  sym->pulp_dots = dots(visual_area ^ (rectangle_16(3, 4) << sym->core_shift), &(sym->pulp_count));
  sym->core_idx = odd_four_core_idx;
  size_t size = 1 << 24;
  sym->pulp_ops = xmalloc(size * sizeof(mirror_op_t));
  sym->core_map = xmalloc(size * sizeof(size_t));
  sym->core_m = 0;
  sym->black_core = xmalloc(ODD_FOUR_CORE_SIZE * sizeof(stones_t));
  sym->white_core = xmalloc(ODD_FOUR_CORE_SIZE * sizeof(stones_t));
  for (size_t idx = 0; idx < size; ++idx) {
    stones_t c = idx;
    stones_t black = c & 7;
    c >>= 3;
    black |= (c & 7) << V_SHIFT_16;
    c >>= 3;
    black |= (c & 7) << (2 * V_SHIFT_16);
    c >>= 3;
    black |= (c & 7) << (3 * V_SHIFT_16);
    c >>= 3;

    stones_t white = c & 7;
    c >>= 3;
    white |= (c & 7) << V_SHIFT_16;
    c >>= 3;
    white |= (c & 7) << (2 * V_SHIFT_16);
    c >>= 3;
    white |= (c & 7) << (3 * V_SHIFT_16);

    // Skip overlapping
    if (black & white) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }

    // Skip illegal
    c = rectangle_16(5, 4);
    int num_chains = 0;
    stones_t *cs = chains_16(black << 1, &num_chains);
    for (int i = 0; i < num_chains; ++i) {
      if (!liberties_16(cs[i], c & ~(white << 1))) {
        sym->pulp_ops[idx] = UCHAR_MAX;
        sym->core_map[idx] = SIZE_MAX;
        free(cs);
        goto next_idx;
      }
    }
    free(cs);

    cs = chains_16(white << 1, &num_chains);
    for (int i = 0; i < num_chains; ++i) {
      if (!liberties_16(cs[i], c & ~(black << 1))) {
        sym->pulp_ops[idx] = UCHAR_MAX;
        sym->core_map[idx] = SIZE_MAX;
        free(cs);
        goto next_idx;
      }
    }
    free(cs);

#ifdef CHECK_SYM_SANITY
    assert(sym->core_idx(black, white) == idx);
#endif

    sym->pulp_ops[idx] = least_of_2(stones_mirror_v_16, stones_mirror_h_w3, &black, &white);
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

  for (size_t i = 0; i < ODD_FOUR_CORE_SIZE; ++i) {
    sym->black_core[i] <<= sym->core_shift;
    sym->white_core[i] <<= sym->core_shift;
  }
}
