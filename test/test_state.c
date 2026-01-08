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

int main() {
  test_rectangle_six_no_liberties_capture_mainline();
  test_rectangle_six_no_liberties_capture_refutation();
  test_rectangle_six_keyspace();
  test_parse();
  test_external_liberties();
  return EXIT_SUCCESS;
}
