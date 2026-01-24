#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/state.h"

void test_external() {
  state s = parse_state(" \
        . w @ . w B x x x \
        w w w w w + x x x \
        w w w w w + x x x \
  ");
  print_state(&s);
  float scr = score(&s);
  printf("Score = %f\n", scr);
  assert(scr == -12);

  s = parse_state("   \
    . w @ . w B x x x \
    w w w w w B x x x \
    w w w w w B x x x \
  ");
  print_state(&s);
  scr = score(&s);
  printf("Score = %f\n", scr);
  assert(scr == -9);

  s = parse_state("   \
    . w . w w B x x x \
    w w w w w B x x x \
    w w w w w B x x x \
  ");
  print_state(&s);
  scr = score(&s);
  printf("Score = %f\n", scr);
  assert(scr == -12);
}

int main() {
  test_external();
  return EXIT_SUCCESS;
}
