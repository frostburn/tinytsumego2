#pragma once

#include <stdlib.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

/**
 * @file symmetry.h
 * @brief Symmetry-reduction helpers for small-board key spaces.
 *
 * These routines make 9x2, 10x2, 11x2, 12x2, 6x3, 7x3, 8x3, 5x4, 6x4, and 5x5
 * problem spaces tractable by reducing out spatial and color symmetries.
 */

/** @brief No mirror operation. */
#define MIRROR_NONE (0)
/** @brief Horizontal mirror operation flag. */
#define MIRROR_H (1)
/** @brief Vertical mirror operation flag. */
#define MIRROR_V (2)
/** @brief Diagonal mirror operation flag. */
#define MIRROR_D (4)

/** @brief Packed mirror-operation bit flags. */
typedef unsigned char mirror_op_t;

/** @brief Function pointer type for mirroring without snapping. */
typedef stones_t (*mirror_f)(stones_t stones);

/** @brief Function pointer type for indexing reduced cores. */
typedef size_t (*core_idx_f)(stones_t black, stones_t white);

/** @brief Precomputed symmetry data for a root state. */
typedef struct symmetry {
  /** @brief Available vertical mirror, or `NULL` when unsupported. */
  mirror_f vertical;
  /** @brief Available horizontal mirror, or `NULL` when unsupported. */
  mirror_f horizontal;
  /** @brief Available diagonal mirror, or `NULL` when unsupported. */
  mirror_f diagonal;

  /** @brief Translation needed to align the symmetry core. */
  int core_shift;

  /** @brief Indexer for the symmetry core used to determine the correct mirrors for the pulp. */
  core_idx_f core_idx;

  /** @brief Number of points outside the reduced core. */
  int pulp_count;
  /** @brief One-bit bitboards for the pulp points. */
  stones_t *pulp_dots;

  /** @brief Precomputed mirror operations for each pulp point. */
  mirror_op_t *pulp_ops;

  /** @brief Mapping from non-reduced core indices to canonical indices. */
  size_t *core_map;

  /** @brief Modulus used when building canonical keys. */
  size_t core_m;

  /** @brief Canonical black core representatives. */
  stones_t *black_core;
  /** @brief Canonical white core representatives. */
  stones_t *white_core;

  /** @brief Pulp block conversion data for black stones. */
  int num_blocks;
  stones_t **black_blocks;
  /** @brief Pulp block conversion data for white stones. */
  stones_t **white_blocks;

  /** @brief Size of the symmetry-reduced key space. */
  size_t size;
} symmetry;

/** @brief Mirror specialized 9x2-style bitboards vertically. */
stones_t stones_mirror_v_2(const stones_t stones);
/** @brief Mirror specialized 9x3-style bitboards vertically. */
stones_t stones_mirror_v_3(const stones_t stones);
/** @brief Mirror specialized 9x4-style bitboards vertically. */
stones_t stones_mirror_v_4(const stones_t stones);
/** @brief Mirror specialized 9x5-style bitboards vertically. */
stones_t stones_mirror_v_5(const stones_t stones);
/** @brief Mirror specialized 9x6-style bitboards vertically. */
stones_t stones_mirror_v_6(stones_t stones);

/** @brief Mirror specialized 2x7-style bitboards horizontally. */
stones_t stones_mirror_h_2(const stones_t stones);
/** @brief Mirror specialized 3x6-style bitboards horizontally. */
stones_t stones_mirror_h_3(const stones_t stones);
/** @brief Mirror specialized 4x5-style bitboards horizontally. */
stones_t stones_mirror_h_4(const stones_t stones);
/** @brief Mirror specialized 5x5-style bitboards horizontally. */
stones_t stones_mirror_h_5(const stones_t stones);
/** @brief Mirror specialized 6x4-style bitboards horizontally. */
stones_t stones_mirror_h_6(stones_t stones);
/** @brief Mirror specialized 7x3-style bitboards horizontally. */
stones_t stones_mirror_h_7(stones_t stones);
/** @brief Mirror specialized 8x3-style bitboards horizontally. */
stones_t stones_mirror_h_8(const stones_t stones);

/** @brief Mirror specialized 3x3-capable bitboards diagonally. */
stones_t stones_mirror_d_3(const stones_t stones);
/** @brief Mirror specialized 4x4-capable bitboards diagonally. */
stones_t stones_mirror_d_4(const stones_t stones);
/** @brief Mirror specialized 5x5-capable bitboards diagonally. */
stones_t stones_mirror_d_5(const stones_t stones);
/** @brief Mirror specialized 6x6-capable bitboards diagonally. */
stones_t stones_mirror_d_6(const stones_t stones);

/** @brief Vertical mirror helper for alternate-width symmetric boards. */
stones_t stones_mirror_v_w2(const stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 3. */
stones_t stones_mirror_h_w3(const stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 4. */
stones_t stones_mirror_h_w4(const stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 5. */
stones_t stones_mirror_h_w5(const stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 6. */
stones_t stones_mirror_h_w6(stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 7. */
stones_t stones_mirror_h_w7(const stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 8. */
stones_t stones_mirror_h_w8(const stones_t stones);
/** @brief Horizontal mirror helper for alternate-width bitboards with width 9. */
stones_t stones_mirror_h_w9(stones_t stones);

/** @brief Compute symmetry reduction data for a root state. */
symmetry compute_symmetry(const state *s);

/** @brief Map black/white bitboards to a canonical symmetry-reduced key. */
size_t to_symmetric_bw_key(const symmetry *sym, const stones_t black, const stones_t white);

/** @brief Recover canonical black/white bitboards from a symmetry-reduced key. */
void from_symmetric_bw_key(const symmetry *sym, size_t key, stones_t *black, stones_t *white);

/** @brief Release allocations owned by a symmetry descriptor. */
void free_symmetry(symmetry *sym);
