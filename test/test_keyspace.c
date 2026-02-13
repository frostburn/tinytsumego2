#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "jkiss/jkiss.h"
#include "tinytsumego2/keyspace.h"

void test_empty() {
  bool indicator(size_t key) {
    key++;
    return false;
  }
  monotonic_compressor mc = create_monotonic_compressor(10, indicator);
  size_t key = compress_key(&mc, 0);
  assert(key == 0);
  key = decompress_key(&mc, 0);
  assert(key == 0);
  for (size_t i = 0; i < 10; ++i) {
    assert(!has_key(&mc, i));
  }
  free_monotonic_compressor(&mc);
}

void test_full() {
  bool indicator(size_t key) {
    return key < 1000;
  }
  monotonic_compressor mc = create_monotonic_compressor(10, indicator);
  for (size_t i = 0; i < 10; ++i) {
    assert(i == decompress_key(&mc, i));
    assert(i == compress_key(&mc, i));
    assert(has_key(&mc, i));
  }
  free_monotonic_compressor(&mc);
}

void test_false_start() {
  bool indicator(size_t key) {
    return key > 0;
  }
  monotonic_compressor mc = create_monotonic_compressor(10, indicator);
  printf("cp = %zu\n", mc.checkpoints[0]);
  for (size_t i = 0; i < 10; ++i) {
    printf("%d\n", mc.deltas[i]);
  }

  for (size_t i = 0; i < 10; ++i) {
    if (indicator(i)) {
      assert(compress_key(&mc, i) == i - 1);
      assert(has_key(&mc, i));
    } else {
      assert(!has_key(&mc, i));
    }
    if (i < 9) {
      assert(decompress_key(&mc, i) == i + 1);
    }
  }
  free_monotonic_compressor(&mc);
}

void test_monotonic() {
  size_t size = 1 << 12;
  bool *flags = malloc(size * sizeof(bool));
  for (size_t i = 0; i < size; ++i) {
    flags[i] = jrand() & 1;
  }
  bool indicator(size_t key) {
    return flags[key];
  }

  monotonic_compressor mc = create_monotonic_compressor(size, indicator);
  printf("factor = %g %%\n", mc.factor * 100);

  for (size_t key = 0; key < 10; ++key) {
    printf("#%zu: %d, %d\n", key, flags[key], mc.deltas[key]);
  }
  for (size_t key = 0; key < mc.size; ++key) {
    size_t uncompressed = decompress_key(&mc, key);
    assert(uncompressed < size);
    assert(flags[uncompressed]);
    size_t recovered = compress_key(&mc, uncompressed);
    assert(recovered == key);
  }
  for (size_t key = 0; key < size; ++key) {
    if (flags[key]) {
      assert(has_key(&mc, key));
    } else {
      assert(!has_key(&mc, key));
    }
  }

  free(flags);
  free_monotonic_compressor(&mc);
}

int main() {
  jkiss_init();
  test_empty();
  test_full();
  test_false_start();
  test_monotonic();
  return 0;
}
