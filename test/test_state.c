#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/state.h"

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
  assert(size == 183708);

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
  assert(size == 734832);

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

  return EXIT_SUCCESS;
}
