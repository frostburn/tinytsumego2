#include <assert.h>
#include <stdlib.h>
#include "tinytsumego2/state.h"

void test_rectangle_six_no_liberties_capture_mainline() {
  state s;
  move_result r;

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

  print_state(&s, false);

  r = make_move(&s, single(1, 1));
  assert(r == NORMAL);
  print_state(&s, true);

  r = make_move(&s, single(1, 0));
  assert(r == NORMAL);
  print_state(&s, false);

  r = make_move(&s, single(0, 1));
  assert(r == NORMAL);
  print_state(&s, true);

  r = make_move(&s, single(2, 1));
  assert(r == NORMAL);
  print_state(&s, false);

  r = make_move(&s, single(2, 0));
  assert(r == TAKE_TARGET);
  print_state(&s, true);
}

void test_rectangle_six_no_liberties_capture_refutation() {
  state s;
  move_result r;

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

  print_state(&s, false);

  r = make_move(&s, single(1, 0));
  assert(r == NORMAL);
  assert(s.ko_threats == 1);
  print_state(&s, true);

  r = make_move(&s, single(1, 1));
  assert(r == NORMAL);
  print_state(&s, false);

  r = make_move(&s, single(2, 0));
  assert(r == NORMAL);
  print_state(&s, true);

  r = make_move(&s, single(0, 0));
  assert(r == NORMAL);
  print_state(&s, false);

  r = make_move(&s, single(0, 1));
  assert(r == NORMAL);
  print_state(&s, true);

  // Legal ko move due to logical "external" threat
  r = make_move(&s, single(0, 0));
  assert(r == KO_THREAT_AND_RETAKE);
  assert(s.ko_threats == 0);
  print_state(&s, false);

  // Illegal ko move
  r = make_move(&s, single(0, 1));
  assert(r == ILLEGAL);
  // Redo: Pass to clear ko and take button
  r = make_move(&s, pass());
  assert(r == CLEAR_KO);
  assert(s.passes == 0);
  assert(s.button == -1);
  print_state(&s, true);

  r = make_move(&s, single(2, 1));
  assert(r == NORMAL);
  assert(s.button == 1);
  print_state(&s, false);

  r = make_move(&s, single(1, 0));
  assert(r == NORMAL);
  print_state(&s, true);

  r = make_move(&s, single(2, 0));
  assert(r == NORMAL);
  print_state(&s, false);

  // Make sure there are no more moves left for Black
  for (int i = 0; i < 64; ++i) {
    r = make_move(&s, 1ULL << i);
    assert(r == ILLEGAL);
  }

  r = make_move(&s, pass());
  assert(r == PASS);
  assert(s.passes == 1);
  print_state(&s, true);

  r = make_move(&s, pass());
  assert(r == PASS);
  assert(s.passes == 2);
  print_state(&s, false);
}

int main() {
  test_rectangle_six_no_liberties_capture_mainline();
  test_rectangle_six_no_liberties_capture_refutation();
  return EXIT_SUCCESS;
}
