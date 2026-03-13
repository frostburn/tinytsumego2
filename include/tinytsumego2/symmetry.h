#pragma once

#include <stdlib.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

// Symmetry reduction utilities to make 9x2, (10x2), (11x2), (12x2), 6x3, (7x3), 8x3, 5x4, 6x4 and 5x5 gobans tractable
// Gobans in parenthesis are still work-in-progress

// Bit flags for operations on the pulp i.e. stones outside the core used for symmetry detection/reduction
#define MIRROR_NONE (0)
#define MIRROR_H (1)
#define MIRROR_V (2)
#define MIRROR_D (4)

typedef unsigned char mirror_op_t;

// Function pointer type for mirroring without snapping
typedef stones_t (*mirror_f)(stones_t stones);

// Function pointer for reducing/manipulating keyspace based on a small core of stones
typedef size_t (*core_idx_f)(stones_t black, stones_t white);

typedef struct symmetry {
  // The available symmetries as function pointers. (NULL if symmetry not available)
  mirror_f vertical;
  mirror_f horizontal;
  mirror_f diagonal;

  // Core translation
  int core_shift;

  // Indexer for core mapping and pulp mirroring
  core_idx_f core_idx;

  // Stones outside of the core
  int pulp_count;
  stones_t *pulp_dots;

  // Pre-calculated mirrors to apply to stones outside of the core
  mirror_op_t *pulp_ops;

  // Pre-calculated mapping from non-reduced core indices to unique canonical indices
  size_t *core_map;

  // Core modulus for key construction
  size_t core_m;

  // Canonical representatives
  stones_t *black_core;
  stones_t *white_core;

  // Pulp block conversion
  int num_blocks;
  stones_t **black_blocks;
  stones_t **white_blocks;

  // Keyspace size
  size_t size;
} symmetry;

stones_t stones_mirror_v_2(const stones_t stones);
stones_t stones_mirror_v_3(const stones_t stones);
stones_t stones_mirror_v_4(const stones_t stones);
stones_t stones_mirror_v_5(const stones_t stones);
stones_t stones_mirror_v_6(stones_t stones);

stones_t stones_mirror_h_2(const stones_t stones);
stones_t stones_mirror_h_3(const stones_t stones);
stones_t stones_mirror_h_4(const stones_t stones);
stones_t stones_mirror_h_5(const stones_t stones);
stones_t stones_mirror_h_6(stones_t stones);
stones_t stones_mirror_h_7(stones_t stones);
stones_t stones_mirror_h_8(stones_t stones);

stones_t stones_mirror_d_3(const stones_t stones);
stones_t stones_mirror_d_4(const stones_t stones);
stones_t stones_mirror_d_5(const stones_t stones);
stones_t stones_mirror_d_6(const stones_t stones);

stones_t stones_mirror_v_w2(const stones_t stones);
stones_t stones_mirror_h_w4(const stones_t stones);
stones_t stones_mirror_h_w5(const stones_t stones);
stones_t stones_mirror_h_w6(stones_t stones);
stones_t stones_mirror_h_w7(const stones_t stones);
stones_t stones_mirror_h_w8(stones_t stones);
stones_t stones_mirror_h_w9(stones_t stones);

symmetry compute_symmetry(const state *s);

size_t to_symmetric_bw_key(const symmetry *sym, const stones_t black, const stones_t white);

void from_symmetric_bw_key(const symmetry *sym, size_t key, stones_t *black, stones_t *white);

void free_symmetry(symmetry *sym);
