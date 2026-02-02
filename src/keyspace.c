#include <stdio.h>
#include "tinytsumego2/keyspace.h"

tight_keyspace create_tight_keyspace(const state *root) {
  tight_keyspace result = {0};

  result.root = *root;
  result.size = tight_keyspace_size(root);

  result.white_to_play = root->white_to_play;
  result.ko_m = abs(root->ko_threats) + 1;

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

  result.player_external_m = popcount(root->external & root->player) + 1;
  result.opponent_external_m = popcount(root->external & root->opponent) + 1;

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
  size_t key = ((!!s->white_to_play) << 1) | s->button;
  key = key * tks->ko_m + abs(s->ko_threats);

  for (int i = tks->num_tritters - 1; i >= 0; --i) {
    key *= tks->tritters[i].m;
    key += TRITS8_TABLE[(s->player >> tks->tritters[i].shift) & tks->tritters[i].mask];
    key += TRITS8_TABLE[(s->opponent >> tks->tritters[i].shift) & tks->tritters[i].mask] << 1;
  }

  // For simplicity we assume that external liberties belonging to a given player form a contiguous chain
  stones_t player = tks->white_to_play == s->white_to_play ? s->player : s->opponent;
  stones_t opponent = tks->white_to_play == s->white_to_play ? s->opponent : s->player;
  key = key * tks->player_external_m + popcount(s->external & player);
  key = key * tks->opponent_external_m + popcount(s->external & opponent);

  return key;
}

void free_tight_keyspace(tight_keyspace *tks) {
  tks->num_tritters = 0;
  free(tks->tritters);
  tks->tritters = NULL;
}
