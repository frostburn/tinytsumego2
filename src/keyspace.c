#include <limits.h>
#include <stdio.h>
#include "tinytsumego2/keyspace.h"
#include "tinytsumego2/util.h"

#define TRIT_BLOCK_SIZE (8)
#define TRIT_BLOCK_M (6561)

tight_keyspace create_tight_keyspace(const state *root, const bool symmetric_threats) {
  tight_keyspace result = {0};

  result.root = *root;
  result.size = tight_keyspace_size(root, symmetric_threats);

  result.symmetric_threats = symmetric_threats;
  if (symmetric_threats) {
    result.ko_m = 2 * abs(root->ko_threats) + 1;
  } else {
    result.ko_m = abs(root->ko_threats) + 1;
  }

  stones_t effective_area = root->logical_area & ~(root->target | root->immortal | root->external);

  int run_length = 0;
  size_t m = 1;
  int shift = -1;
  // Intentional 65th bit to trigger collection
  for (int i = 0; i < 65; ++i) {
    const stones_t p = 1ULL << i;
    if (p & effective_area) {
      run_length++;
      m *= 3;
      if (shift < 0) {
        shift = i;
      }
    }
    if (run_length >= 8 || (run_length && !(p & effective_area))) {
      result.num_tritters++;
      result.tritters = realloc(result.tritters, result.num_tritters * sizeof(tritter));
      result.tritters[result.num_tritters - 1] = (tritter) {m, shift, (1 << run_length) - 1};

      run_length = 0;
      m = 1;
      shift = -1;
    }
  }

  stones_t root_black = root->white_to_play ? root->opponent : root->player;
  stones_t root_white = root->white_to_play ? root->player : root->opponent;
  result.black_external_m = popcount(root->external & root_black) + 1;
  result.white_external_m = popcount(root->external & root_white) + 1;

  // Key inversion
  result.prefix_m = 4 * result.ko_m * result.black_external_m * result.white_external_m;
  result.prefixes = malloc(result.prefix_m * sizeof(state));
  for (size_t key = 0; key < result.prefix_m; ++key) {
    result.prefixes[key] = from_tight_key(root, key, symmetric_threats);
  }

  int effective_size = popcount(effective_area);
  result.num_blocks = ceil_div(effective_size, TRIT_BLOCK_SIZE);
  result.black_blocks = malloc(result.num_blocks * sizeof(stones_t*));
  result.white_blocks = malloc(result.num_blocks * sizeof(stones_t*));
  m = result.prefix_m;
  int i;
  for (i = 0; i < effective_size / TRIT_BLOCK_SIZE; i++) {
    result.black_blocks[i] = malloc(TRIT_BLOCK_M * sizeof(stones_t));
    result.white_blocks[i] = malloc(TRIT_BLOCK_M * sizeof(stones_t));
    for (size_t j = 0; j < TRIT_BLOCK_M; ++j) {
      state s = from_tight_key(root, j * m, symmetric_threats);
      result.black_blocks[i][j] = s.player & effective_area;
      result.white_blocks[i][j] = s.opponent & effective_area;
    }
    m *= TRIT_BLOCK_M;
  }
  size_t tail_m = 1;
  for (int i = 0; i < effective_size % TRIT_BLOCK_SIZE; ++i) {
    tail_m *= 3;
  }
  if (tail_m != 1) {
    result.black_blocks[i] = malloc(tail_m * sizeof(stones_t));
    result.white_blocks[i] = malloc(tail_m * sizeof(stones_t));
    for (size_t j = 0; j < tail_m; ++j) {
      state s = from_tight_key(root, j * m, symmetric_threats);
      result.black_blocks[i][j] = s.player & effective_area;
      result.white_blocks[i][j] = s.opponent & effective_area;
    }
  }

  return result;
}

// Magic array that converts bits to trits
static const size_t TRITS8_TABLE[] = {
  0, 1, 3, 4, 9, 10, 12, 13, 27, 28, 30, 31, 36, 37, 39, 40, 81, 82, 84, 85, 90, 91, 93, 94, 108, 109, 111,
  112, 117, 118, 120, 121, 243, 244, 246, 247, 252, 253, 255, 256, 270, 271, 273, 274, 279, 280, 282, 283,
  324, 325, 327, 328, 333, 334, 336, 337, 351, 352, 354, 355, 360, 361, 363, 364, 729, 730, 732, 733, 738,
  739, 741, 742, 756, 757, 759, 760, 765, 766, 768, 769, 810, 811, 813, 814, 819, 820, 822, 823, 837, 838,
  840, 841, 846, 847, 849, 850, 972, 973, 975, 976, 981, 982, 984, 985, 999, 1000, 1002, 1003, 1008, 1009,
  1011, 1012, 1053, 1054, 1056, 1057, 1062, 1063, 1065, 1066, 1080, 1081, 1083, 1084, 1089, 1090, 1092, 1093,
  2187, 2188, 2190, 2191, 2196, 2197, 2199, 2200, 2214, 2215, 2217, 2218, 2223, 2224, 2226, 2227, 2268, 2269,
  2271, 2272, 2277, 2278, 2280, 2281, 2295, 2296, 2298, 2299, 2304, 2305, 2307, 2308, 2430, 2431, 2433, 2434,
  2439, 2440, 2442, 2443, 2457, 2458, 2460, 2461, 2466, 2467, 2469, 2470, 2511, 2512, 2514, 2515, 2520, 2521,
  2523, 2524, 2538, 2539, 2541, 2542, 2547, 2548, 2550, 2551, 2916, 2917, 2919, 2920, 2925, 2926, 2928, 2929,
  2943, 2944, 2946, 2947, 2952, 2953, 2955, 2956, 2997, 2998, 3000, 3001, 3006, 3007, 3009, 3010, 3024, 3025,
  3027, 3028, 3033, 3034, 3036, 3037, 3159, 3160, 3162, 3163, 3168, 3169, 3171, 3172, 3186, 3187, 3189, 3190,
  3195, 3196, 3198, 3199, 3240, 3241, 3243, 3244, 3249, 3250, 3252, 3253, 3267, 3268, 3270, 3271, 3276, 3277,
  3279, 3280
};

size_t to_tight_key_fast(const tight_keyspace *tks, const state *s) {
  size_t key = 0;

  stones_t black = s->white_to_play ? s->opponent : s->player;
  stones_t white = s->white_to_play ? s->player : s->opponent;

  for (int i = tks->num_tritters - 1; i >= 0; --i) {
    key *= tks->tritters[i].m;
    key += TRITS8_TABLE[(black >> tks->tritters[i].shift) & tks->tritters[i].mask];
    key += TRITS8_TABLE[(white >> tks->tritters[i].shift) & tks->tritters[i].mask] << 1;
  }

  // For simplicity we assume that external liberties belonging to a given player form a contiguous chain
  key = key * tks->black_external_m + popcount(s->external & black);
  key = key * tks->white_external_m + popcount(s->external & white);

  if (tks->symmetric_threats) {
    key = key * tks->ko_m + s->ko_threats + abs(tks->root.ko_threats);
  } else {
    key = key * tks->ko_m + abs(s->ko_threats);
  }
  key = (key << 2) | ((s->button) << 1) | !!s->white_to_play;

  return key;
}

state from_tight_key_fast(const tight_keyspace *tks, size_t key) {
  state result = tks->prefixes[key % tks->prefix_m];
  key /= tks->prefix_m;
  for (int i = 0; i < tks->num_blocks; ++i) {
    size_t k = key % TRIT_BLOCK_M;
    if (result.white_to_play) {
      result.player |= tks->white_blocks[i][k];
      result.opponent |= tks->black_blocks[i][k];
    } else {
      result.player |= tks->black_blocks[i][k];
      result.opponent |= tks->white_blocks[i][k];
    }
    key /= TRIT_BLOCK_M;
  }
  return result;
}

void free_tight_keyspace(tight_keyspace *tks) {
  tks->num_tritters = 0;
  free(tks->tritters);
  tks->tritters = NULL;

  free(tks->prefixes);
  tks->prefixes = NULL;

  for (int i = 0; i < tks->num_blocks; ++i) {
    free(tks->black_blocks[i]);
    free(tks->white_blocks[i]);
  }
  free(tks->black_blocks);
  tks->black_blocks = NULL;
  free(tks->white_blocks);
  tks->white_blocks = NULL;
}

monotonic_compressor create_monotonic_compressor(size_t num_keys, indicator_f indicator) {
  monotonic_compressor result = {0};
  result.uncompressed_size = num_keys;
  result.num_checkpoints = ceil_divz(num_keys, 1 << CHAR_BIT);
  result.checkpoints = malloc(result.num_checkpoints * sizeof(size_t));
  result.deltas = malloc(num_keys * sizeof(unsigned char));

  size_t num_legal = 0;
  size_t last_checkpoint = 0;
  for (size_t key = 0; key < num_keys; ++key) {
    if ((key & 255) == 0) {
      result.checkpoints[key >> CHAR_BIT] = num_legal;
      last_checkpoint = num_legal;
    }
    result.deltas[key] = num_legal - last_checkpoint;
    if (indicator(key)) {
      num_legal++;
    }
  }

  result.size = num_legal;
  if (num_legal) {
    result.factor = (double)(num_keys) / (double)(num_legal);
  } else {
    result.factor = 1;
  }

  return result;
}

size_t compress_key(const monotonic_compressor *mc, const size_t key) {
  return mc->checkpoints[key >> CHAR_BIT] + (size_t)mc->deltas[key];
}

size_t decompress_key(const monotonic_compressor *mc, const size_t compressed_key) {
  if (!mc->size) {
    return 0;
  }
  size_t result = (size_t)(compressed_key * mc->factor);
  while ((result + 1) < mc->uncompressed_size && compress_key(mc, result) <= compressed_key) {
    result++;
  }
  while (compress_key(mc, result) > compressed_key) {
    result--;
  }
  return result;
}

bool has_key(const monotonic_compressor *mc, const size_t key) {
  if (key == mc->uncompressed_size - 1) {
    return compress_key(mc, key) == mc->size - 1;
  }
  if ((key & 255) == 255) {
    return compress_key(mc, key) != compress_key(mc, key + 1);
  }
  return mc->deltas[key] != mc->deltas[key + 1];
}

void free_monotonic_compressor (monotonic_compressor *mc) {
  free(mc->checkpoints);
  mc->checkpoints = NULL;
  free(mc->deltas);
  mc->deltas = NULL;
}

compressed_keyspace create_compressed_keyspace(const state *root) {
  compressed_keyspace result = {0};
  result.keyspace = create_tight_keyspace(root, true);
  result.prefix_m = result.keyspace.prefix_m / result.keyspace.black_external_m / result.keyspace.white_external_m;
  bool indicator(size_t key) {
    const state s = from_tight_key_fast(&(result.keyspace), key * result.prefix_m);
    return is_legal(&s);
  }
  result.compressor = create_monotonic_compressor(result.keyspace.size / result.prefix_m, indicator);
  result.size = result.prefix_m * result.compressor.size;
  return result;
}

size_t to_compressed_key(const compressed_keyspace *cks, const state *s) {
  const size_t key = to_tight_key_fast(&(cks->keyspace), s);
  return (key % cks->prefix_m) + cks->prefix_m * compress_key(&(cks->compressor), key / cks->prefix_m);
}

state from_compressed_key(const compressed_keyspace *cks, size_t key) {
  key = (key % cks->prefix_m) + cks->prefix_m * decompress_key(&(cks->compressor), key / cks->prefix_m);
  return from_tight_key_fast(&(cks->keyspace), key);
}

size_t remap_tight_key(const compressed_keyspace *cks, size_t key) {
  return (key % cks->prefix_m) + cks->prefix_m * compress_key(&(cks->compressor), key / cks->prefix_m);
}

bool was_legal(const compressed_keyspace *cks, size_t key) {
  return has_key(&(cks->compressor), key / cks->prefix_m);
}

void free_compressed_keyspace(compressed_keyspace *cks) {
  free_tight_keyspace(&(cks->keyspace));
  free_monotonic_compressor(&(cks->compressor));
}
