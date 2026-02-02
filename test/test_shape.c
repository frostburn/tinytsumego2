#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/shape.h"

void test_notchers_n_1() {
  state s;
  state expected;

  s = notcher("112NS");
  expected = parse_state("          \
    , , W W W , , , x x x x x x x x \
    , W . b b W W , x x x x x x x x \
    W W @ . b b W W x x x x x x x x \
    . . . . . . . . x x x x x x x x \
  ");
  expected.wide = true;

  print_state(&s);
  assert(equals(&s, &expected));

  s = notcher("135SW");
  expected = parse_state("          \
    , , , , W W W , , , , , , x x x \
    , W W W b b W W W W W W , x x x \
    W W b b b . b b b b b W W x x x \
    . . . . . . . . . . . . . x x x \
  ");
  expected.wide = true;

  print_state(&s);
  assert(equals(&s, &expected));
}

void test_notchers_n_2() {
  state s;
  state expected;

  s = notcher("212NN");
  expected = parse_state("          \
    , , W W W W , , , x x x x x x x \
    , W . b b . W W , x x x x x x x \
    W W @ . . b b W W x x x x x x x \
    . . . . . . . . . x x x x x x x \
  ");
  expected.wide = true;

  print_state(&s);
  assert(equals(&s, &expected));

  s = notcher("264WS");
  expected = parse_state("          \
    , , , , , , , W W W W , , , , , \
    , W W W W W W W b b b W W W W , \
    W W b b b b b b . . b b b b W W \
    . . . . . . . . . . . . . . . . \
  ");
  expected.wide = true;

  print_state(&s);
  assert(equals(&s, &expected));
}

int main() {
  test_notchers_n_1();
  test_notchers_n_2();
  return EXIT_SUCCESS;
}
