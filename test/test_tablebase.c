#include <assert.h>
#include <stdio.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/tablebase.h"

void test_two_eyes() {
  state s = parse_state(" \
        . b . b W x x x x \
        b b b b W x x x x \
        b b - W W x x x x \
        b b - W x x x x x \
        b b - W x x x x x \
  ");
  print_state(&s);

  size_t key = to_corner_tablebase_key(&s);

  state t = from_corner_tablebase_key(key);

  print_state(&t);

  printf("%zu\n", key);

  assert(key < TABLEBASE_SIZE);
}

int main() {
  test_two_eyes();
  return EXIT_SUCCESS;
}
