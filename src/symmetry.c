#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include "tinytsumego2/symmetry.h"
#include "tinytsumego2/util.h"

#define CHECK_SYM_SANITY

#define TRIT_BLOCK_SIZE (8)
#define TRIT_BLOCK_M (6561)

stones_t stones_mirror_v_2(const stones_t stones) {
  return ((stones & H0) << V_SHIFT) | ((stones & H1) >> V_SHIFT);
}

stones_t stones_mirror_v_3(const stones_t stones) {
  return (
    ((stones & H0) << (2 * V_SHIFT)) |
    (stones & H1) |
    ((stones & H2) >> (2 * V_SHIFT))
  );
}

stones_t stones_mirror_v_4(const stones_t stones) {
    return (
      ((stones & H0) << (3 * V_SHIFT)) |
      ((stones & H1) << V_SHIFT) |
      ((stones & H2) >> V_SHIFT) |
      ((stones & H3) >> (3 * V_SHIFT))
  );
}

stones_t stones_mirror_v_5(const stones_t stones) {
  return (
    ((stones & H0) << (4 * V_SHIFT)) |
    ((stones & H1) << (2 * V_SHIFT)) |
    (stones & H2) |
    ((stones & H3) >> (2 * V_SHIFT)) |
    ((stones & H4) >> (4 * V_SHIFT))
  );
}

stones_t stones_mirror_v_6(stones_t stones) {
  stones = (
    ((stones & (H0 | H1 | H2)) << (3 * V_SHIFT)) |
    ((stones & (H3 | H4 | H5)) >> (3 * V_SHIFT))
  );
  return (
    ((stones & (H0 | H3)) << (2 * V_SHIFT)) |
    (stones & (H1 | H4)) |
    ((stones & (H2 | H5)) >> (2 * V_SHIFT))
  );
}

stones_t stones_mirror_h_2(const stones_t stones) {
  return ((stones & V0) << H_SHIFT) | ((stones & V1) >> H_SHIFT);
}

stones_t stones_mirror_h_3(const stones_t stones) {
  return (
    ((stones & V0) << (2 * H_SHIFT)) |
    (stones & V1) |
    ((stones & V2) >> (2 * H_SHIFT))
  );
}

stones_t stones_mirror_h_4(const stones_t stones) {
    return (
      ((stones & V0) << (3 * H_SHIFT)) |
      ((stones & V1) << H_SHIFT) |
      ((stones & V2) >> H_SHIFT) |
      ((stones & V3) >> (3 * H_SHIFT))
  );
}

stones_t stones_mirror_h_5(const stones_t stones) {
  return (
    ((stones & V0) << (4 * H_SHIFT)) |
    ((stones & V1) << (2 * H_SHIFT)) |
    (stones & V2) |
    ((stones & V3) >> (2 * H_SHIFT)) |
    ((stones & V4) >> (4 * H_SHIFT))
  );
}

stones_t stones_mirror_h_6(stones_t stones) {
  stones = (
    ((stones & (V0 | V1 | V2)) << (3 * H_SHIFT)) |
    ((stones & (V3 | V4 | V5)) >> (3 * H_SHIFT))
  );
  return (
    ((stones & (V0 | V3)) << (2 * H_SHIFT)) |
    (stones & (V1 | V4)) |
    ((stones & (V2 | V5)) >> (2 * H_SHIFT))
  );
}

stones_t stones_mirror_h_7(stones_t stones) {
  stones = (
    ((stones >> (4 * H_SHIFT)) & (V0 | V1 | V2)) |
    (stones & V3) |
    ((stones & (V0 | V1 | V2)) << (4 * H_SHIFT))
  );
  return (
    ((stones >> (2 * H_SHIFT)) & (V0 | V4)) |
    (stones & (V1 | V3 | V5)) |
    ((stones & (V0 | V4)) << (2 * H_SHIFT))
  );
}

stones_t stones_mirror_h_8(stones_t stones) {
  stones = (
    ((stones & (V0 | V1 | V2 | V3)) << (4 * H_SHIFT)) |
    ((stones & (V4 | V5 | V6 | V7)) >> (4 * H_SHIFT))
  );
  stones = (
    ((stones & (V0 | V1 | V4 | V5)) << (2 * H_SHIFT)) |
    ((stones & (V2 | V3 | V6 | V7)) >> (2 * H_SHIFT))
  );
  return (
    ((stones & (V0 | V2 | V4 | V6)) << H_SHIFT) |
    ((stones & (V1 | V3 | V5 | V7)) >> H_SHIFT)
  );
}

stones_t stones_mirror_d_3(const stones_t stones) {
  return (
    (stones & D0) |
    ((stones & D1) << D_SHIFT) |
    ((stones >> D_SHIFT) & D1) |
    ((stones & D2) << (2 * D_SHIFT)) |
    ((stones >> (2 * D_SHIFT)) & D2)
  );
}

stones_t stones_mirror_d_4(const stones_t stones) {
  return (
    (stones & D0) |
    ((stones & D1) << D_SHIFT) |
    ((stones >> D_SHIFT) & D1) |
    ((stones & D2) << (2 * D_SHIFT)) |
    ((stones >> (2 * D_SHIFT)) & D2) |
    ((stones & D3) << (3 * D_SHIFT)) |
    ((stones >> (3 * D_SHIFT)) & D3)
  );
}

stones_t stones_mirror_d_5(const stones_t stones) {
  return (
    (stones & D0) |
    ((stones & D1) << D_SHIFT) |
    ((stones >> D_SHIFT) & D1) |
    ((stones & D2) << (2 * D_SHIFT)) |
    ((stones >> (2 * D_SHIFT)) & D2) |
    ((stones & D3) << (3 * D_SHIFT)) |
    ((stones >> (3 * D_SHIFT)) & D3) |
    ((stones & D4) << (4 * D_SHIFT)) |
    ((stones >> (4 * D_SHIFT)) & D4)
  );
}

stones_t stones_mirror_d_6(const stones_t stones) {
  return (
    (stones & D0) |
    ((stones & D1) << D_SHIFT) |
    ((stones >> D_SHIFT) & D1) |
    ((stones & D2) << (2 * D_SHIFT)) |
    ((stones >> (2 * D_SHIFT)) & D2) |
    ((stones & D3) << (3 * D_SHIFT)) |
    ((stones >> (3 * D_SHIFT)) & D3) |
    ((stones & D4) << (4 * D_SHIFT)) |
    ((stones >> (4 * D_SHIFT)) & D4) |
    ((stones & D5) << (5 * D_SHIFT)) |
    ((stones >> (5 * D_SHIFT)) & D5)
  );
}

mirror_op_t least_of_2(mirror_f vertical, mirror_f horizontal, stones_t *black, stones_t *white) {
  mirror_op_t op = MIRROR_NONE;

  stones_t cb = vertical(*black);
  stones_t cw = vertical(*white);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_V;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_V;
  }
  cb = horizontal(cb);
  cw = horizontal(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_V | MIRROR_H;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_V | MIRROR_H;
  }
  cb = vertical(cb);
  cw = vertical(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_H;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_H;
  }
  return op;
}

mirror_op_t least_of_3(mirror_f vertical, mirror_f horizontal, mirror_f diagonal, stones_t *black, stones_t *white) {
  mirror_op_t op = MIRROR_NONE;

  stones_t cb = vertical(*black);
  stones_t cw = vertical(*white);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_V;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_V;
  }
  cb = horizontal(cb);
  cw = horizontal(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_V | MIRROR_H;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_V | MIRROR_H;
  }
  cb = vertical(cb);
  cw = vertical(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_H;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_H;
  }
  cb = diagonal(cb);
  cw = diagonal(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_H | MIRROR_D;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_H | MIRROR_D;
  }
  // Diagonal mirror flip-commutes with cardinal mirrors
  cb = horizontal(cb);
  cw = horizontal(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_V | MIRROR_H | MIRROR_D;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_V | MIRROR_H | MIRROR_D;
  }
  cb = vertical(cb);
  cw = vertical(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_V | MIRROR_D;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_V | MIRROR_D;
  }
  cb = horizontal(cb);
  cw = horizontal(cw);
  if (cb < *black) {
    *black = cb;
    *white = cw;
    op = MIRROR_D;
  } else if (cb == *black && cw < *white) {
    *white = cw;
    op = MIRROR_D;
  }
  return op;
}

// @ @ @
// @ . @
// @ . @
// @ @ @
// wide = false

// Compression ratio ~ 3.92 (vs. ideal 4.0)
#define ODD_EVEN_CORE_SIZE (15066)

size_t odd_even_core_idx(stones_t black, stones_t white) {
  size_t result = black & 7;
  black >>= V_SHIFT;
  result |= (black & 1) << 3;
  result |= (black & 4) << 2;
  black >>= V_SHIFT;
  result |= (black & 1) << 5;
  result |= (black & 4) << 4;
  black >>= V_SHIFT;
  result |= (black & 7) << 7;

  result |= (white & 7) << 10;
  white >>= V_SHIFT;
  result |= (white & 1) << 13;
  result |= (white & 4) << 12;
  white >>= V_SHIFT;
  result |= (white & 1) << 15;
  result |= (white & 4) << 14;
  white >>= V_SHIFT;
  result |= (white & 7) << 17;

  return result;
}

size_t even_odd_core_idx(stones_t black, stones_t white) {
  return odd_even_core_idx(stones_mirror_d_4(black), stones_mirror_d_4(white));
}

void prepare_odd_even_symmetry(symmetry *sym, stones_t visual_area) {
  stones_t core_mask = rectangle(3, 4) ^ (rectangle(1, 2) << (H_SHIFT + V_SHIFT));
  sym->pulp_dots = dots(visual_area ^ (core_mask << sym->core_shift), &(sym->pulp_count));
  sym->core_idx = odd_even_core_idx;
  size_t size = 1 << 20;
  sym->pulp_ops = malloc(size * sizeof(mirror_op_t));
  sym->core_map = malloc(size * sizeof(size_t));
  sym->core_m = 0;
  sym->black_core = malloc(ODD_EVEN_CORE_SIZE * sizeof(stones_t));
  sym->white_core = malloc(ODD_EVEN_CORE_SIZE * sizeof(stones_t));
  for (size_t idx = 0; idx < size; ++idx) {
    stones_t c = idx;
    stones_t black = c & 7;
    c >>= 3;
    black |= (c & 1) << V_SHIFT;
    black |= (c & 2) << (V_SHIFT + H_SHIFT);
    c >>= 2;
    black |= (c & 1) << (2 * V_SHIFT);
    black |= (c & 2) << (2 * V_SHIFT + H_SHIFT);
    c >>= 2;
    black |= (c & 7) << (3 * V_SHIFT);
    c >>= 3;

    stones_t white = c & 7;
    c >>= 3;
    white |= (c & 1) << V_SHIFT;
    white |= (c & 2) << (V_SHIFT + H_SHIFT);
    c >>= 2;
    white |= (c & 1) << (2 * V_SHIFT);
    white |= (c & 2) << (2 * V_SHIFT + H_SHIFT);
    c >>= 2;
    white |= (c & 7) << (3 * V_SHIFT);

    // Skip overlapping
    if (black & white) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }

    #ifdef CHECK_SYM_SANITY
      assert(sym->core_idx(black, white) == idx);
    #endif

    sym->pulp_ops[idx] = least_of_2(stones_mirror_v_4, stones_mirror_h_3, &black, &white);
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

  for (size_t i = 0; i < ODD_EVEN_CORE_SIZE; ++i) {
    sym->black_core[i] <<= sym->core_shift;
    sym->white_core[i] <<= sym->core_shift;
  }
}

// @ @ @
// @ @ @
// @ @ @
// wide = false

// Compression ratio ~ 6.98 (vs. ideal 8.0)
#define ODD_SQUARE_CORE_SIZE (2820)

size_t odd_square_core_idx(stones_t black, stones_t white) {
  size_t result = black & 7;
  black >>= V_SHIFT;
  result |= (black & 7) << 3;
  black >>= V_SHIFT;
  result |= (black & 7) << 6;

  result |= (white & 7) << 9;
  white >>= V_SHIFT;
  result |= (white & 7) << 12;
  white >>= V_SHIFT;
  result |= (white & 7) << 15;

  return result;
}

void prepare_odd_square_symmetry(symmetry *sym, stones_t visual_area) {
  sym->pulp_dots = dots(visual_area ^ (rectangle(3, 3) << sym->core_shift), &(sym->pulp_count));
  sym->core_idx = odd_square_core_idx;
  size_t size = 1 << 18;
  sym->pulp_ops = malloc(size * sizeof(mirror_op_t));
  sym->core_map = malloc(size * sizeof(size_t));
  sym->core_m = 0;
  sym->black_core = malloc(ODD_SQUARE_CORE_SIZE * sizeof(stones_t));
  sym->white_core = malloc(ODD_SQUARE_CORE_SIZE * sizeof(stones_t));
  for (size_t idx = 0; idx < size; ++idx) {
    stones_t c = idx;
    stones_t black = c & 7;
    c >>= 3;
    black |= (c & 7) << V_SHIFT;
    c >>= 3;
    black |= (c & 7) << (2 * V_SHIFT);
    c >>= 3;

    stones_t white = c & 7;
    c >>= 3;
    white |= (c & 7) << V_SHIFT;
    c >>= 3;
    white |= (c & 7) << (2 * V_SHIFT);

    // Skip overlapping
    if (black & white) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }

    // Skip illegal
    c = single(1, 1) & black;
    if (c && !liberties(c, ~white)) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }
    c = single(1, 1) & white;
    if (c && !liberties(c, ~black)) {
      sym->pulp_ops[idx] = UCHAR_MAX;
      sym->core_map[idx] = SIZE_MAX;
      continue;
    }

    #ifdef CHECK_SYM_SANITY
      assert(sym->core_idx(black, white) == idx);
    #endif

    sym->pulp_ops[idx] = least_of_3(stones_mirror_v_3, stones_mirror_h_3, stones_mirror_d_3, &black, &white);
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

  for (size_t i = 0; i < ODD_SQUARE_CORE_SIZE; ++i) {
    sym->black_core[i] <<= sym->core_shift;
    sym->white_core[i] <<= sym->core_shift;
  }
}

symmetry compute_symmetry(const state *s) {
  symmetry result = {0};
  assert(!s->wide);
  int w = width_of(s->visual_area);
  int h = height_of(s->visual_area);
  switch (w) {
    case 2:
      assert(false && "Width of 2 not implemented. Try flipping input diagonally.");
      break;
    case 3:
      result.horizontal = stones_mirror_h_3;
      break;
    case 4:
      result.horizontal = stones_mirror_h_4;
      break;
    case 5:
      result.horizontal = stones_mirror_h_5;
      result.core_shift += H_SHIFT;
      break;
    case 6:
      result.horizontal = stones_mirror_h_6;
      result.core_shift += H_SHIFT;
      break;
    case 7:
      result.horizontal = stones_mirror_h_7;
      result.core_shift += 2 * H_SHIFT;
      break;
    case 8:
      result.horizontal = stones_mirror_h_8;
      result.core_shift += 2 * H_SHIFT;
      break;
    case 9:
      result.horizontal = stones_mirror_h;
      result.core_shift += 3 * H_SHIFT;
      break;
    default:
      assert(false && "Unsupported width");
      break;
  }
  switch (h) {
    case 2:
      result.vertical = stones_mirror_v_2;
      break;
    case 3:
      result.vertical = stones_mirror_v_3;
      break;
    case 4:
      result.vertical = stones_mirror_v_4;
      break;
    case 5:
      result.vertical = stones_mirror_v_5;
      result.core_shift += V_SHIFT;
      break;
    case 6:
      result.vertical = stones_mirror_v_6;
      result.core_shift += V_SHIFT;
      break;
    case 7:
      result.vertical = stones_mirror_v;
      result.core_shift += 2 * V_SHIFT;
      break;
    default:
      assert(false && "Unsupported height");
      break;
  }

  if (w == h) {
    switch (w) {
      case 3:
        result.diagonal = stones_mirror_d_3;
        break;
      case 4:
        result.diagonal = stones_mirror_d_4;
        break;
      case 5:
        result.diagonal = stones_mirror_d_5;
        break;
      case 6:
        result.diagonal = stones_mirror_d_6;
        break;
      case 7:
        result.diagonal = stones_mirror_d;
        break;
      default:
        assert(false && "Unsupported square size");
        break;
    }
    if (w & 1) {
      prepare_odd_square_symmetry(&result, s->visual_area);
    } else {
      assert(false && "Even square core not implemented yet");
    }
  } else {
    if (w & 1) {
      if (h & 1) {
        assert(false && "Odd-odd core not implemented yet");
      } else {
        prepare_odd_even_symmetry(&result, s->visual_area);
      }
    } else {
      if (h & 1) {
        // Flip
        result.core_shift = (result.core_shift / V_SHIFT) * H_SHIFT + (result.core_shift % V_SHIFT) * V_SHIFT;
        prepare_odd_even_symmetry(&result, stones_mirror_d(s->visual_area));
        // Flip back
        result.core_shift = (result.core_shift / V_SHIFT) * H_SHIFT + (result.core_shift % V_SHIFT) * V_SHIFT;
        for (int i = 0; i < result.pulp_count; ++i) {
          result.pulp_dots[i] = stones_mirror_d(result.pulp_dots[i]);
        }
        for (size_t i = 0; i < (1 << 20); ++i) {
          if (result.pulp_ops[i] == UCHAR_MAX) {
            continue;
          }
          result.pulp_ops[i] = (MIRROR_H * !!(result.pulp_ops[i] & MIRROR_V)) | (MIRROR_V * !!(result.pulp_ops[i] & MIRROR_H));
        }
        for (size_t i = 0; i < ODD_EVEN_CORE_SIZE; ++i) {
          result.black_core[i] = stones_mirror_d(result.black_core[i]);
          result.white_core[i] = stones_mirror_d(result.white_core[i]);
        }
        result.core_idx = even_odd_core_idx;
      } else {
        assert(false && "Even-even cores not implemented yet");
      }
    }
  }

  result.num_blocks = ceil_div(result.pulp_count, TRIT_BLOCK_SIZE);
  result.black_blocks = malloc(result.num_blocks * sizeof(stones_t*));
  result.white_blocks = malloc(result.num_blocks * sizeof(stones_t*));

  size_t m = 1;
  int i;
  for (i = 0; i < result.pulp_count / TRIT_BLOCK_SIZE; ++i) {
    result.black_blocks[i] = calloc(TRIT_BLOCK_M, sizeof(stones_t));
    result.white_blocks[i] = calloc(TRIT_BLOCK_M, sizeof(stones_t));
    for (size_t j = 0; j < TRIT_BLOCK_M; ++j) {
      size_t key = m * j;
      for (int k = result.pulp_count - 1; k >= 0; --k) {
        switch (key % 3) {
          case 1:
            result.black_blocks[i][j] |= result.pulp_dots[k];
            break;
          case 2:
            result.white_blocks[i][j] |= result.pulp_dots[k];
            break;
        }
        key /= 3;
      }
    }
    m *= TRIT_BLOCK_M;
  }

  size_t tail_m = 1;
  for (int j = 0; j < result.pulp_count % TRIT_BLOCK_SIZE; ++j) {
    tail_m *= 3;
  }
  if (tail_m != 1) {
    result.black_blocks[i] = calloc(tail_m, sizeof(stones_t));
    result.white_blocks[i] = calloc(tail_m, sizeof(stones_t));
    for (size_t j = 0; j < tail_m; ++j) {
      size_t key = m * j;
      for (int k = result.pulp_count - 1; k >= 0; --k) {
        switch (key % 3) {
          case 1:
            result.black_blocks[i][j] |= result.pulp_dots[k];
            break;
          case 2:
            result.white_blocks[i][j] |= result.pulp_dots[k];
            break;
        }
        key /= 3;
      }
    }
  }

  result.size = 1;
  for (int i = 0; i < result.pulp_count; ++i) {
    result.size *= 3;
  }
  result.size *= result.core_m;

  return result;
}

size_t to_symmetric_bw_key(const symmetry *sym, stones_t black, stones_t white) {
  size_t idx = sym->core_idx(black >> sym->core_shift, white >> sym->core_shift);
  mirror_op_t op = sym->pulp_ops[idx];
  if (op & MIRROR_V) {
    black = sym->vertical(black);
    white = sym->vertical(white);
  }
  if (op & MIRROR_H) {
    black = sym->horizontal(black);
    white = sym->horizontal(white);
  }
  if (op & MIRROR_D) {
    black = sym->diagonal(black);
    white = sym->diagonal(white);
  }
  size_t key = 0;
  for (int i = 0; i < sym->pulp_count; ++i) {
    key *= 3;
    if (sym->pulp_dots[i] & black) {
      key += 1;
    } else if (sym->pulp_dots[i] & white) {
      key += 2;
    }
  }
  return sym->core_map[idx] + sym->core_m * key;
}

void from_symmetric_bw_key(const symmetry *sym, size_t key, stones_t *black, stones_t *white) {
  size_t k = key % sym->core_m;
  key /= sym->core_m;
  *black = sym->black_core[k];
  *white = sym->white_core[k];
  for (int i = 0; i < sym->num_blocks; ++i) {
    size_t k = key % TRIT_BLOCK_M;
    *black |= sym->black_blocks[i][k];
    *white |= sym->white_blocks[i][k];
    key /= TRIT_BLOCK_M;
  }
}

void free_symmetry(symmetry *sym) {
  free(sym->pulp_dots);
  sym->pulp_count = 0;
  sym->pulp_dots = NULL;
  free(sym->pulp_ops);
  sym->pulp_ops = NULL;
  free(sym->core_map);
  sym->core_map = NULL;
  free(sym->black_core);
  sym->black_core = NULL;
  free(sym->white_core);
  sym->white_core = NULL;
  for (int i = 0; i < sym->num_blocks; ++i) {
    free(sym->black_blocks[i]);
    free(sym->white_blocks[i]);
  }
  free(sym->black_blocks);
  sym->black_blocks = NULL;
  free(sym->white_blocks);
  sym->white_blocks = NULL;
}
