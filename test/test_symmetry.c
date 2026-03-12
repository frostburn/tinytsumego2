#include <assert.h>
#include <stdio.h>
#include "tinytsumego2/symmetry.h"
#include "jkiss/jkiss.h"

void test_3x4() {
  state root = {0};
  root.visual_area = rectangle(3, 4);
  root.logical_area = root.visual_area;
  symmetry sym = compute_symmetry(&root);
  printf("Core modulus = %zu\n", sym.core_m);
  assert(sym.core_m == 15066);
  printf("Keyspace size = %zu\n", sym.size);
  assert(sym.size == 135594);
  assert(sym.pulp_count == 2);
  stones_t pulp_mask = 0ULL;
  for (int i = 0; i < sym.pulp_count; ++i) {
    pulp_mask |= sym.pulp_dots[i];
  }
  print_stones(pulp_mask);
  printf("%llu\n", pulp_mask);
  assert(pulp_mask == 525312ULL);

  for (int i = 0; i < 10; ++i) {
    state s = root;
    s.player = jlrand() & s.visual_area;
    s.opponent = jlrand() & s.visual_area & ~s.player;
    print_state(&s);
    size_t key = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu\n", key);
    state s0 = s;
    s0.player = 0;
    s0.opponent = 0;
    from_symmetric_bw_key(&sym, key, &(s0.player), &(s0.opponent));
    print_state(&s0);
    state s1 = s0;
    mirror_v(&s1);
    snap(&s1);
    state s2 = s0;
    mirror_h(&s2);
    snap(&s2);
    state s3 = s1;
    mirror_h(&s3);
    snap(&s3);
    assert(equals(&s, &s0) || equals(&s, &s1) || equals(&s, &s2) || equals(&s, &s3));
  }

  for (int i = 0; i < 10; ++i) {
    size_t key = jlrand() % sym.size;
    state s = root;
    from_symmetric_bw_key(&sym, key, &(s.player), &(s.opponent));
    print_state(&s);
    size_t k = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu ?= %zu\n", key, k);
    if (key != k) {
      from_symmetric_bw_key(&sym, k, &(s.player), &(s.opponent));
      print_state(&s);
    }
    assert(k == key);
  }

  free_symmetry(&sym);
}

void test_5x4() {
  state root = {0};
  root.visual_area = rectangle(5, 4);
  root.logical_area = root.visual_area;
  symmetry sym = compute_symmetry(&root);
  printf("Core modulus = %zu\n", sym.core_m);
  assert(sym.core_m == 15066);
  printf("Keyspace size = %zu\n", sym.size);
  assert(sym.size == 889632234);
  assert(sym.pulp_count == 10);
  stones_t pulp_mask = 0ULL;
  for (int i = 0; i < sym.pulp_count; ++i) {
    pulp_mask |= sym.pulp_dots[i];
  }
  print_stones(pulp_mask);
  printf("%llu\n", pulp_mask);
  assert(pulp_mask == 2287217169ULL);

  for (int i = 0; i < 10; ++i) {
    state s = root;
    s.player = jlrand() & s.visual_area;
    s.opponent = jlrand() & s.visual_area & ~s.player;
    print_state(&s);
    size_t key = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu\n", key);
    state s0 = s;
    s0.player = 0;
    s0.opponent = 0;
    from_symmetric_bw_key(&sym, key, &(s0.player), &(s0.opponent));
    print_state(&s0);
    state s1 = s0;
    mirror_v(&s1);
    snap(&s1);
    state s2 = s0;
    mirror_h(&s2);
    snap(&s2);
    state s3 = s1;
    mirror_h(&s3);
    snap(&s3);
    assert(equals(&s, &s0) || equals(&s, &s1) || equals(&s, &s2) || equals(&s, &s3));
  }

  for (int i = 0; i < 10; ++i) {
    size_t key = jlrand() % sym.size;
    state s = root;
    from_symmetric_bw_key(&sym, key, &(s.player), &(s.opponent));
    print_state(&s);
    size_t k = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu ?= %zu\n", key, k);
    if (key != k) {
      from_symmetric_bw_key(&sym, k, &(s.player), &(s.opponent));
      print_state(&s);
    }
    assert(k == key);
  }

  free_symmetry(&sym);
}

void test_5x5() {
  state root = {0};
  root.visual_area = rectangle(5, 5);
  root.logical_area = root.visual_area;
  symmetry sym = compute_symmetry(&root);
  printf("Core modulus = %zu\n", sym.core_m);
  assert(sym.core_m == 2820);
  printf("Keyspace size = %zu\n", sym.size);
  assert(sym.size == 121391753220LL);
  assert(sym.pulp_count == 16);
  stones_t pulp_mask = 0ULL;
  for (int i = 0; i < sym.pulp_count; ++i) {
    pulp_mask |= sym.pulp_dots[i];
  }
  print_stones(pulp_mask);
  printf("%llu\n", pulp_mask);
  assert(pulp_mask == 2132589945375ULL);

  for (int i = 0; i < 10; ++i) {
    state s = root;
    s.player = jlrand() & s.visual_area;
    s.opponent = jlrand() & s.visual_area & ~s.player;
    if (s.player & single(2, 2)) {
      if (!liberties(single(2, 2), ~s.opponent)) {
        i--;
        continue;
      }
    }
    if (s.opponent & single(2, 2)) {
      if (!liberties(single(2, 2), ~s.player)) {
        i--;
        continue;
      }
    }
    print_state(&s);
    size_t key = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu\n", key);
    state s0 = s;
    s0.player = 0;
    s0.opponent = 0;
    from_symmetric_bw_key(&sym, key, &(s0.player), &(s0.opponent));
    print_state(&s0);
    state s1 = s0;
    mirror_v(&s1);
    snap(&s1);
    state s2 = s0;
    mirror_h(&s2);
    snap(&s2);
    state s3 = s1;
    mirror_h(&s3);
    snap(&s3);

    state s4 = s0;
    mirror_d(&s4);
    snap(&s4);
    state s5 = s4;
    mirror_v(&s5);
    snap(&s5);
    state s6 = s4;
    mirror_h(&s6);
    snap(&s6);
    state s7 = s5;
    mirror_h(&s7);
    snap(&s7);
    assert(equals(&s, &s0) || equals(&s, &s1) || equals(&s, &s2) || equals(&s, &s3) || equals(&s, &s4) || equals(&s, &s5) || equals(&s, &s6) || equals(&s, &s7));
  }

  for (int i = 0; i < 10; ++i) {
    size_t key = jlrand() % sym.size;
    state s = root;
    from_symmetric_bw_key(&sym, key, &(s.player), &(s.opponent));
    print_state(&s);
    size_t k = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu ?= %zu\n", key, k);
    if (key != k) {
      from_symmetric_bw_key(&sym, k, &(s.player), &(s.opponent));
      print_state(&s);
    }
    assert(k == key);
  }

  free_symmetry(&sym);
}

void test_3x6() {
  state root = {0};
  root.visual_area = rectangle(3, 6);
  root.logical_area = root.visual_area;
  symmetry sym = compute_symmetry(&root);
  printf("Core modulus = %zu\n", sym.core_m);
  assert(sym.core_m == 15066);
  printf("Keyspace size = %zu\n", sym.size);
  assert(sym.size == 98848026);
  assert(sym.pulp_count == 8);
  stones_t pulp_mask = 0ULL;
  for (int i = 0; i < sym.pulp_count; ++i) {
    pulp_mask |= sym.pulp_dots[i];
  }
  print_stones(pulp_mask);
  printf("%llu\n", pulp_mask);
  assert(pulp_mask == 246290873581575ULL);

  for (int i = 0; i < 10; ++i) {
    state s = root;
    s.player = jlrand() & s.visual_area;
    s.opponent = jlrand() & s.visual_area & ~s.player;
    print_state(&s);
    size_t key = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu\n", key);
    state s0 = s;
    s0.player = 0;
    s0.opponent = 0;
    from_symmetric_bw_key(&sym, key, &(s0.player), &(s0.opponent));
    print_state(&s0);
    state s1 = s0;
    mirror_v(&s1);
    snap(&s1);
    state s2 = s0;
    mirror_h(&s2);
    snap(&s2);
    state s3 = s1;
    mirror_h(&s3);
    snap(&s3);
    assert(equals(&s, &s0) || equals(&s, &s1) || equals(&s, &s2) || equals(&s, &s3));
  }

  for (int i = 0; i < 10; ++i) {
    size_t key = jlrand() % sym.size;
    state s = root;
    from_symmetric_bw_key(&sym, key, &(s.player), &(s.opponent));
    print_state(&s);
    size_t k = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu ?= %zu\n", key, k);
    if (key != k) {
      from_symmetric_bw_key(&sym, k, &(s.player), &(s.opponent));
      print_state(&s);
    }
    assert(k == key);
  }

  free_symmetry(&sym);
}

void test_6x3() {
  state root = {0};
  root.visual_area = rectangle(6, 3);
  root.logical_area = root.visual_area;
  symmetry sym = compute_symmetry(&root);
  printf("Core modulus = %zu\n", sym.core_m);
  assert(sym.core_m == 15066);
  printf("Keyspace size = %zu\n", sym.size);
  assert(sym.size == 98848026);
  assert(sym.pulp_count == 8);
  stones_t pulp_mask = 0ULL;
  for (int i = 0; i < sym.pulp_count; ++i) {
    pulp_mask |= sym.pulp_dots[i];
  }
  print_stones(pulp_mask);
  printf("%llu\n", pulp_mask);
  assert(pulp_mask == 8673825ULL);

  for (int i = 0; i < 10; ++i) {
    state s = root;
    s.player = jlrand() & s.visual_area;
    s.opponent = jlrand() & s.visual_area & ~s.player;
    print_state(&s);
    size_t key = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu\n", key);
    state s0 = s;
    s0.player = 0;
    s0.opponent = 0;
    from_symmetric_bw_key(&sym, key, &(s0.player), &(s0.opponent));
    print_state(&s0);
    state s1 = s0;
    mirror_v(&s1);
    snap(&s1);
    state s2 = s0;
    mirror_h(&s2);
    snap(&s2);
    state s3 = s1;
    mirror_h(&s3);
    snap(&s3);
    assert(equals(&s, &s0) || equals(&s, &s1) || equals(&s, &s2) || equals(&s, &s3));
  }

  for (int i = 0; i < 10; ++i) {
    size_t key = jlrand() % sym.size;
    state s = root;
    from_symmetric_bw_key(&sym, key, &(s.player), &(s.opponent));
    print_state(&s);
    size_t k = to_symmetric_bw_key(&sym, s.player, s.opponent);
    printf("key = %zu ?= %zu\n", key, k);
    if (key != k) {
      from_symmetric_bw_key(&sym, k, &(s.player), &(s.opponent));
      print_state(&s);
    }
    assert(k == key);
  }

  free_symmetry(&sym);
}

int main() {
  jkiss_init();
  test_3x4();
  test_5x4();
  test_5x5();
  test_3x6();
  test_6x3();
  return 0;
}
