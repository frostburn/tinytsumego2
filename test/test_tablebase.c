#include <assert.h>
#include <stdio.h>
#include "jkiss/jkiss.h"
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

void test_corner_keys() {
  for (int i = 0; i < 10; ++i) {
    size_t key = jrand() % TABLEBASE_SIZE;
    state s = from_corner_tablebase_key(key);
    print_state(&s);
    size_t recovered = to_corner_tablebase_key(&s);
    printf("%zu ?= %zu\n", key, recovered);
    assert(key == recovered);
  }
}

void test_edge_keys() {
  for (int i = 0; i < 10; ++i) {
    size_t key = jrand() % TABLEBASE_SIZE;
    state s = from_edge_tablebase_key(key);
    print_state(&s);
    size_t recovered = to_edge_tablebase_key(&s);
    printf("%zu ?= %zu\n", key, recovered);
    if (offset_h_16(s.logical_area) == 1) {
      assert(key == recovered);
    }
  }
}

int main() {
  jkiss_init();
  test_two_eyes();
  test_corner_keys();
  test_edge_keys();
  return EXIT_SUCCESS;
}
