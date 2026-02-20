#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/keyspace.h"

state rectangle_six() {
  state s;

  s.visual_area = rectangle(4, 3);
  s.logical_area = s.visual_area;
  s.player = 0;
  s.opponent = rectangle(4, 3) ^ rectangle(3, 2);
  s.ko = 0;
  s.target = s.opponent;
  s.immortal = 0;
  s.external = 0;
  s.passes = 0;
  s.ko_threats = -1;
  s.button = 0;
  s.white_to_play = false;

  return s;
}

state bent_four_in_the_corner_is_dead() {
  state s;

  s.visual_area = rectangle(4, 4);
  s.logical_area = rectangle(3, 1) | rectangle(1, 3);
  s.player = single(1, 0) | single(0, 1);
  s.opponent = s.visual_area ^ s.logical_area;
  s.ko = 0;
  s.target = s.opponent;
  s.immortal = 0;
  s.external = 0;
  s.passes = 0;
  s.ko_threats = -1;
  s.button = 0;
  s.white_to_play = false;

  return s;
}

state bent_four_in_the_corner() {
  state s = (state){0};

  s.visual_area = rectangle(6, 3);
  s.external = single(4, 0) | single(4, 1);
  s.logical_area = rectangle(3, 1) | single(0, 1) | s.external;
  s.opponent = (rectangle(2, 3) << 4);
  s.player = rectangle(4, 3) ^ s.logical_area ^ s.external;
  s.immortal = s.opponent ^ s.external;
  s.opponent |= single(1, 0);
  s.target = s.player;
  s.white_to_play = true;
  s.ko_threats = 1;

  return s;
}

state straight_nine_wide() {
  state s = {0};
  s.visual_area = rectangle_16(10, 2);
  s.logical_area = rectangle_16(9, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.player = s.target;
  s.wide = true;
  return s;
}

void test_rectangle_six_no_liberties_capture_mainline() {
  state s = rectangle_six();
  move_result r;

  print_state(&s);

  r = make_move(&s, single(1, 1));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(1, 0));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(0, 1));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(2, 1));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(2, 0));
  assert(r == TAKE_TARGET);
  print_state(&s);
}

void test_rectangle_six_no_liberties_capture_refutation() {
  state s = rectangle_six();
  move_result r;

  print_state(&s);

  r = make_move(&s, single(1, 0));
  assert(r == NORMAL);
  assert(s.ko_threats == 1);
  print_state(&s);

  r = make_move(&s, single(1, 1));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(2, 0));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(0, 0));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(0, 1));
  assert(r == NORMAL);
  print_state(&s);

  // Legal ko move due to logical "external" threat
  r = make_move(&s, single(0, 0));
  assert(r == KO_THREAT_AND_RETAKE);
  assert(s.ko_threats == 0);
  print_state(&s);

  // Illegal ko move
  r = make_move(&s, single(0, 1));
  assert(r == ILLEGAL);
  assert(s.white_to_play == false);
  // Redo: Pass to clear ko and take button
  r = make_move(&s, pass());
  assert(r == CLEAR_KO);
  assert(s.passes == 0);
  assert(s.button == -1);
  print_state(&s);

  r = make_move(&s, single(2, 1));
  assert(r == NORMAL);
  assert(s.button == 1);
  print_state(&s);

  r = make_move(&s, single(1, 0));
  assert(r == NORMAL);
  print_state(&s);

  r = make_move(&s, single(2, 0));
  assert(r == NORMAL);
  print_state(&s);

  // Make sure there are no more moves left for Black
  for (int i = 0; i < 64; ++i) {
    r = make_move(&s, 1ULL << i);
    assert(r == ILLEGAL);
  }

  r = make_move(&s, pass());
  assert(r == PASS);
  assert(s.passes == 1);
  print_state(&s);

  r = make_move(&s, pass());
  assert(r == SECOND_PASS);
  assert(s.passes == 2);
  print_state(&s);
}

void test_rectangle_six_keyspace() {
  state root = rectangle_six();
  size_t size = keyspace_size(&root);
  printf("size = %zu\n", size);
  assert(size == 122472);

  size_t root_key = to_key(&root, &root);

  state child = root;
  make_move(&child, pass());
  size_t child_key = to_key(&root, &child);

  make_move(&child, single(1, 0));
  size_t grandchild_key = to_key(&root, &child);

  assert(root_key < size);
  assert(child_key < size);
  assert(grandchild_key < size);
  assert(root_key != child_key);
  assert(root_key != grandchild_key);
  assert(child_key != grandchild_key);
}

void test_rectangle_six_external_liberties_keyspace() {
  state root = rectangle_six();
  root.visual_area = rectangle(5, 3);
  root.external = rectangle(1, 3) << 4;
  root.logical_area = rectangle(3, 2) | root.external;
  root.player |= root.external;
  print_state(&root);

  size_t size = keyspace_size(&root);
  printf("size = %zu\n", size);
  assert(size == 489888);

  size_t root_key = to_key(&root, &root);

  state child = root;
  make_move(&child, single(4, 1));
  size_t child_key = to_key(&root, &child);

  state alt = root;
  make_move(&alt, single(4, 2));
  size_t alt_key = to_key(&root, &alt);

  move_result r = make_move(&child, single(4, 1));
  assert(r == ILLEGAL);
  size_t undo_key = to_key(&root, &child);

  make_move(&child, single(1, 1));
  size_t grandchild_key = to_key(&root, &child);

  assert(root_key < size);
  assert(child_key < size);
  assert(grandchild_key < size);
  assert(alt_key == child_key);
  assert(undo_key == child_key);
  assert(root_key != child_key);
  assert(root_key != grandchild_key);
  assert(child_key != grandchild_key);
}

void test_parse() {
    char *visuals = "\
      , , , , , , . . . \
      , , W W W W W b . \
      , B B B B B W b . \
      , . . w w w b b 0 \
      , . . . @ @ W W . \
      , , W . . . W , , \
      , , , . W W W , , \
    ";

    const state expected = (state) {
      9223372036854775807ULL,
      146107400938717632ULL,
      3324354560000ULL,
      2020018406396327936ULL,
      0ULL,
      33319616512ULL,
      2020018364536649728ULL,
      0ULL,
      0,
      0,
      0,
      false,
      false
    };

    state s = parse_state(visuals);
    print_state(&s);
    repr_state(&s);
    assert(equals(&s, &expected));
    print_stones(s.visual_area);
    print_stones(s.logical_area);
    print_stones(s.player);
    print_stones(s.opponent);
    print_stones(s.target);
    print_stones(s.immortal);
}

void test_external_liberties() {
  state s = parse_state("\
    b - x x x x x x x\
    b - x x x x x x x\
  ");
  move_result r = make_move(&s, single(1, 0));
  print_state(&s);
  assert(r == ILLEGAL);

  r = make_move(&s, single(1, 1));
  assert(r == ILLEGAL);

  r = make_move(&s, pass());
  print_state(&s);
  assert(r == TAKE_BUTTON);

  state alt = s;

  r = make_move(&s, single(1, 0));
  print_state(&s);
  assert(r == FILL_EXTERNAL);

  r = make_move(&alt, single(1, 1));
  assert(r == FILL_EXTERNAL);
  assert(equals(&s, &alt));  // Make sure target liberties are filled in standard order

  r = make_move(&s, pass());
  print_state(&s);
  assert(r == PASS);

  r = make_move(&s, single(1, 1));
  print_state(&s);
  assert(r == TAKE_TARGET);
}

void test_2x1_occupied() {
  state s = parse_state(". .");
  s.button = 1;

  move_result r = make_move(&s, single(0, 0));
  print_state(&s);
  assert(r == NORMAL);

  r = make_move(&s, single(0, 0));
  assert(r == ILLEGAL);
}

void test_benson() {
  state s = parse_state(" \
        . . . W W W W x x \
        W W W W @ @ W x x \
        W @ @ @ . @ W x x \
        W @ . . @ @ W x x \
        W @ @ @ W W W x x \
        W W W W W , , x x \
  ");

  state expected = s;

  print_state(&s);
  apply_benson(&s);
  assert(equals(&s, &expected));

  s.opponent |= single(2, 3);

  expected = parse_state("\
        . . . W W W W x x \
        W W W W B B W x x \
        W B B B , B W x x \
        W B , B B B W x x \
        W B B B W W W x x \
        W W W W W , , x x \
  ");

  print_state(&expected);

  print_state(&s);
  apply_benson(&s);
  print_state(&s);
  assert(equals(&s, &expected));

  s = bent_four_in_the_corner_is_dead();
  expected = s;
  apply_benson(&s);

  assert(equals(&s, &expected));

  make_move(&s, pass());
  make_move(&s, single(2, 0));
  expected = s;
  apply_benson(&s);
  print_state(&s);

  assert(equals(&s, &expected));

  s = parse_state("   \
    @ . @ w w B x x x \
    . . w . w B x x x \
    w w w w w B x x x \
  ");

  expected = s;
  apply_benson(&s);
  print_state(&s);

  assert(equals(&s, &expected));

  s = parse_state("@ . 0 .");
  print_state(&s);
  apply_benson(&s);
  print_state(&s);

  expected = parse_state(", W W ,");
  assert(equals(&s, &expected));

  s = parse_state(". @ . 0 0");
  expected = s;
  apply_benson(&s);
  print_state(&s);
  assert(equals(&s, &expected));
}

void test_mirror() {
  state s = rectangle_six();
  s.logical_area ^= s.target;
  s.player = single(2, 0);
  mirror_v(&s);
  print_state(&s);

  state expected = parse_state("\
              x x x x x x x x x \
              x x x x x x x x x \
              x x x x x x x x x \
              x x x x x x x x x \
              w w w w x x x x x \
              . . . w x x x x x \
              . . @ w x x x x x \
  ");
  expected.ko_threats = -1;
  print_state(&expected);

  assert(equals(&s, &expected));

  mirror_h(&s);
  print_state(&s);

  expected = parse_state("\
        x x x x x x x x x \
        x x x x x x x x x \
        x x x x x x x x x \
        x x x x x x x x x \
        x x x x x w w w w \
        x x x x x w . . . \
        x x x x x w @ . . \
  ");
  expected.ko_threats = -1;
  assert(equals(&s, &expected));

  assert(!can_mirror_d(&s));

  s = rectangle_six();
  s.logical_area ^= s.target;
  s.player = single(2, 0);
  assert(can_mirror_d(&s));

  mirror_d(&s);
  print_state(&s);

  expected = parse_state("\
        . . w x x x x x x \
        . . w x x x x x x \
        @ . w x x x x x x \
        w w w x x x x x x \
  ");
  expected.ko_threats = -1;
  assert(equals(&s, &expected));
}

void test_snap() {
  state s = bent_four_in_the_corner_is_dead();
  mirror_h(&s);
  print_state(&s);
  snap(&s);
  print_state(&s);

  state expected = parse_state("\
              w . @ . x x x x x \
              w w w @ x x x x x \
              w w w . x x x x x \
              w w w w x x x x x \
  ");
  expected.ko_threats = -1;
  assert(equals(&s, &expected));
}

void test_wide_state() {
  state s = parse_state("\
    , B , , , , , , , , , , x x x x \
    , , B B B B B B B B B , x x x x \
    B B w w w w w w w w B B x x x x \
    . . . . . . . . . . . . x x x x \
  ");
  s.wide = true;
  print_state(&s);

  move_result r = make_move(&s, single_16(2, 3));
  print_state(&s);
  assert(r == NORMAL);

  r = make_move(&s, single_16(3, 3));
  print_state(&s);
  assert(r == NORMAL);

  s = parse_state("\
    . . w x xxxx xxxx xxxx \
    w w w \
  ");
  s.wide = true;
  print_state(&s);

  r = make_move(&s, single_16(0, 0));
  print_state(&s);
  assert(r == NORMAL);

  s = parse_state("\
    W b b 0 . 0 b b W W W x x x x x \
    W W b b b b b b b W W x x x x x \
    , W W W W W W W W , , \
  ");
  s.wide = true;
  print_state(&s);
  apply_benson(&s);
  print_state(&s);
}

void test_immortal_regions() {
  state s = parse_state(" \
        W . W b . . . b W \
        W W W b b b b b W \
  ");
  normalize_immortal_regions(&s, &s);
  print_state(&s);
  assert(!(s.logical_area & single(1, 0)));

  s = parse_state(" \
        . . x W @ . . @ W \
        x x x W @ @ @ @ W \
        x x x W W W W W W \
  ");
  normalize_immortal_regions(&s, &s);
  print_state(&s);
  assert(s.logical_area & single(0, 0));
}

void test_struggle() {
  state s = parse_state("\
    W . . . 0 0 . b b b x x x x x x \
    W W b b b b b b W W x x x x x x \
    , W W W W W W W W , x x x x x x \
  ");
  s.wide = true;
  print_state(&s);
  move_result r = struggle(&s);
  assert(r == TAKE_TARGET);
}

void test_bent_four_keyspace_coverage() {
  state root = bent_four_in_the_corner();
  print_state(&root);
  size_t size = keyspace_size(&root);

  state *states = malloc(100 * sizeof(state));
  states[0] = root;
  int num_states = 1;
  int index = 0;

  while (index < num_states) {
    state parent = states[index++];
    for (int i = 0; i < 64; ++i) {
      state child = parent;
      const move_result r = make_move(&child, 1ULL << i);
      assert(compare(&child, &child) == 0);
      if (r > TAKE_TARGET) {
        bool novel = true;
        for (int j = 0; j < num_states; ++j) {
          if (equals(&child, states + j)) {
            novel = false;
            break;
          }
        }
        if (novel) {
          states[num_states++] = child;
        }
      }
    }
  }

  printf("%d states expanded\n", num_states);
  assert(num_states == 55);

  for (int i = 0; i < num_states; ++i) {
    size_t my_key = to_key(&root, states + i);
    assert(my_key < size);
    for (int j = 0; j < i; ++j) {
      size_t your_key = to_key(&root, states + j);
      if (my_key == your_key) {
        print_state(states + i);
        print_state(states + j);
      }
      assert(compare(states + i, states + j) != 0);
      assert(my_key != your_key);
    }
  }

  free(states);
}

void test_bent_four_debug_keys() {
  state root = (state) {16547391ULL, 8727ULL, 3939336ULL, 12607538ULL, 0ULL, 3939336ULL, 12599328ULL, 8208ULL, 0, 0, 0, true, false};
  state a = (state) {16547391ULL, 519ULL, 12607538ULL, 3939337ULL, 512ULL, 3939336ULL, 12607536ULL, 0ULL, 0, 0, 1, false, false};
  state b = (state) {16547391ULL, 519ULL, 12607538ULL, 3939337ULL, 0ULL, 3939336ULL, 12607536ULL, 0ULL, 1, 0, 1, false, false};

  print_state(&root);
  print_state(&a);
  print_state(&b);

  assert(compare(&a, &b) != 0);
  assert(to_key(&root, &a) != to_key(&root, &b));
}

void test_rectangle_tight_keys() {
  state root = rectangle_six();
  // Normalize representation
  root.logical_area &= ~(root.target | root.immortal);
  print_state(&root);
  const size_t key = to_tight_key(&root, &root, false);
  assert(key < tight_keyspace_size(&root, false));

  state s = from_tight_key(&root, key, false);
  print_state(&s);
  assert(equals(&root, &s));

  state child = root;
  make_move(&child, single(1, 0));
  const size_t child_key = to_tight_key(&root, &child, false);
  state tc = from_tight_key(&root, child_key, false);
  print_state(&tc);
  assert(equals(&child, &tc));

  tight_keyspace tks = create_tight_keyspace(&root, false);
  state fs = from_tight_key_fast(&tks, key);
  print_state(&fs);
  assert(equals(&s, &fs));
  state fc = from_tight_key_fast(&tks, child_key);
  print_state(&fc);
  assert(equals(&child, &fc));

  const size_t fast_key = to_tight_key_fast(&tks, &root);
  assert(key == fast_key);
  const size_t fast_child_key = to_tight_key_fast(&tks, &child);
  assert(child_key == fast_child_key);
  free_tight_keyspace(&tks);
}

void test_bent_four_tight_keyspace() {
  state root = bent_four_in_the_corner();
  print_state(&root);

  tight_keyspace tks = create_tight_keyspace(&root, true);

  size_t num_nodes = tight_keyspace_size(&root, true);

  state *states = malloc(num_nodes * sizeof(state));

  for (size_t i = 0; i < num_nodes; ++i) {
    states[i] = from_tight_key(&root, i, true);
    state s = from_tight_key_fast(&tks, i);
    assert(equals(&s, states + i));
    size_t key = to_tight_key_fast(&tks, states + i);
    assert(key == i);
    for (size_t j = 0; j < i; ++j) {
      if (equals(states + i, states + j)) {
        printf("%zu != %zu\n", i, j);
        from_tight_key(&root, j, true);
        print_state(states + i);
        print_state(states + j);
      }
      assert(!equals(states + i, states + j));
    }
  }
  free(states);
  free_tight_keyspace(&tks);
}

void test_wide_keyspace() {
  state root = straight_nine_wide();
  print_state(&root);

  tight_keyspace tks = create_tight_keyspace(&root, false);

  size_t num_nodes = tight_keyspace_size(&root, false);

  state *states = malloc(num_nodes * sizeof(state));

  for (size_t i = 0; i < num_nodes; ++i) {
    states[i] = from_tight_key(&root, i, false);
    state s = from_tight_key_fast(&tks, i);
    assert(equals(&s, states + i));
    size_t key = to_tight_key_fast(&tks, states + i);
    assert(key == i);
    for (size_t j = 0; j < i; ++j) {
      if (j > 100) {
        break;
      }
      if (equals(states + i, states + j)) {
        printf("%zu != %zu\n", i, j);
        from_tight_key(&root, j, false);
        print_state(states + i);
        print_state(states + j);
      }
      assert(!equals(states + i, states + j));
    }
  }
  free(states);
  free_tight_keyspace(&tks);
}

void test_legality() {
  state s = parse_state(" \
        b b . b b W x x x \
        b b b b b W x x x \
        b b b b b - x x x \
  ");
  stones_t temp = s.player;
  s.player = s.opponent;
  s.opponent = temp;
  s.white_to_play = true;
  s.passes = 1;
  s.ko_threats = 1;
  s.button = 1;

  state expected = (state) {16547391ULL, 8388612ULL, 8405024ULL, 8142363ULL, 0ULL, 8142363ULL, 16416ULL, 8388608ULL, 1, 1, 1, true, false};

  print_state(&s);
  assert(is_legal(&s));
  assert(equals(&s, &expected));

  print_stones(s.external);

  make_move(&s, s.external);
  print_state(&s);
  assert(is_legal(&s));

  state root = (state) {16547391ULL, 8408623ULL, 8138768ULL, 8405024ULL, 0ULL, 8138768ULL, 0ULL, 8405024ULL, 0, -1, -1, false, false};
  tight_keyspace tks = create_tight_keyspace(&root, false);

  s.button = abs(s.button);
  size_t key = to_tight_key_fast(&tks, &s);
  size_t slow_key = to_tight_key(&root, &s, false);
  printf("%zu ?= %zu\n", key, slow_key);

  state f = from_tight_key(&root, key, false);
  print_state(&f);

  assert(is_legal(&f));
}

void test_compressed_keyspace() {
  const state root = rectangle_six();
  print_state(&root);
  compressed_keyspace cks = create_compressed_keyspace(&root);
  printf(
    "size = %zu, factor = %g %% requiring %g + %g bytes of aux space, compare with %zu\n",
    cks.size,
    cks.compressor.factor * 100,
    cks.compressor.size * cks.compressor.factor,
    floor(cks.compressor.size * cks.compressor.factor / 256) * sizeof(size_t),
    cks.keyspace.size
  );

  for (size_t key = 0; key < cks.size; ++key) {
    const state s = from_compressed_key(&cks, key);
    assert(is_legal(&s));
  }
  free_compressed_keyspace(&cks);
}

void test_illegal_ko() {
  state s = parse_state(" \
              @ 0 . . . . . B , \
              * @ . w w B B B , \
              @ w w w B , , , , \
              . B B B , B , , , \
              . B , , , , , , , \
              B B , , , , , , , \
  ");
  print_state(&s);
  const move_result r = make_move(&s, single(0, 1));
  assert(r == ILLEGAL);
  print_state(&s);
}

int main() {
  test_rectangle_six_no_liberties_capture_mainline();
  test_rectangle_six_no_liberties_capture_refutation();
  test_rectangle_six_keyspace();
  test_rectangle_six_external_liberties_keyspace();
  test_parse();
  test_external_liberties();
  test_2x1_occupied();
  test_benson();
  test_mirror();
  test_snap();
  test_wide_state();
  test_immortal_regions();
  test_struggle();
  test_bent_four_keyspace_coverage();
  test_bent_four_debug_keys();
  test_rectangle_tight_keys();
  test_bent_four_tight_keyspace();
  test_wide_keyspace();
  test_legality();
  test_compressed_keyspace();
  test_illegal_ko();

  return EXIT_SUCCESS;
}
