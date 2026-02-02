#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "tinytsumego2/shape.h"

state notcher(const char *code) {
  if (strlen(code) != 5) {
    fprintf(stderr, "Notcher code must be 5 characters long\n");
    exit(EXIT_FAILURE);
  }
  int n = code[0] - '0';
  int a = code[1] - '0';
  int b = code[2] - '0';
  char x = toupper(code[3]);
  char y = toupper(code[4]);

  assert(n >= 0);
  assert(a >= 0);
  assert(b >= 0);
  int width = 2 + a + n + b + 2;
  assert(width <= WIDTH_16);

  stones_t black = 0ULL;
  stones_t white = 0ULL;
  stones_t logical_area = (
    (rectangle_16(width, 1) << (3 * V_SHIFT_16)) |
    (rectangle_16(width - 4, 1) << (2 * H_SHIFT_16 + 2 * V_SHIFT_16))
  );

  // Second line
  for (int i = 0; i < 2; ++i) {
    white |= single_16(i, 2);
  }
  for (int i = 0; i < a; ++i) {
    black |= single_16(2 + i, 2);
  }
  for (int i = 0; i < b; ++i) {
    black |= single_16(2 + a + n + i, 2);
  }
  for (int i = 0; i < 2; ++i) {
    white |= single_16(2 + a + n + b + i, 2);
  }

  // Third line
  for (int i = 0; i < a; ++i) {
    white |= single_16(1 + i, 1);
  }
  stones_t hip = single_16(1 + a, 1);
  switch (x) {
    case 'S':
      black |= hip;
      logical_area |= hip;
      break;
    case 'N':
      logical_area |= hip;
      break;
    case 'W':
      white |= hip;
      break;
    default:
      fprintf(stderr, "Invalid left hip code\n");
      exit(EXIT_FAILURE);
  }
  for (int i = 0; i < n; ++i) {
    black |= single_16(2 + a + i, 1);
    logical_area |= single_16(2 + a + i, 1);
  }
  hip = single_16(2 + a + n, 1);
  switch (y) {
    case 'S':
      black |= hip;
      logical_area |= hip;
      break;
    case 'N':
      logical_area |= hip;
      break;
    case 'W':
      white |= hip;
      break;
    default:
      fprintf(stderr, "Invalid right hip code\n");
      exit(EXIT_FAILURE);
  }
  for (int i = 0; i < b; ++i) {
    white |= single_16(3 + a + n + i, 1);
  }

  // Fourth line
  for (int i = 0; i < n + 2; ++i) {
    white |= single_16(1 + a + i, 0);
  }

  state result = {0};
  result.wide = true;
  result.visual_area = rectangle_16(width, HEIGHT_16);
  result.player = black;
  result.opponent = white;
  result.logical_area = logical_area;
  result.immortal = white;

  int num_chains;
  stones_t *cs = chains_16(black, &num_chains);
  for (int i = 0; i < num_chains; ++i) {
    if (popcount(cs[i]) > 1) {
      result.target |= cs[i];
    }
  }
  free(cs);

  if (!result.target) {
    result.target = black;
  }
  result.logical_area ^= result.target;

  return result;
}
