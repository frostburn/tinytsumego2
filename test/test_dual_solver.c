#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/dual_solver.h"

state bulky_five() {
  state s = {0};
  s.visual_area = rectangle(5, 4);
  s.logical_area = (rectangle(2, 2) << (H_SHIFT + V_SHIFT)) | single(3, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.opponent = s.target;
  return s;
}

state bent_four_in_the_corner_is_dead() {
  state s = {0};
  s.visual_area = rectangle(4, 4);
  s.logical_area = rectangle(3, 1) | rectangle(1, 3);
  s.player = single(1, 0) | single(0, 1);
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;
  s.ko_threats = -1;
  return s;
}

state bent_four_in_the_corner_might_be_seki() {
  state s = {0};
  s.visual_area = rectangle(4, 3);
  s.logical_area = rectangle(3, 2) ^ single(1, 1);
  s.player = s.visual_area ^ s.logical_area;
  s.opponent = rectangle(3, 1);
  s.target = s.player;
  s.ko_threats = 1;
  return s;
}

state dead_three() {
  state s = {0};
  s.visual_area = rectangle(4, 3);
  s.logical_area = rectangle(3, 1) << V_SHIFT;
  s.player = s.visual_area ^ s.logical_area;
  s.opponent = single(1, 1);
  s.target = s.player;
  s.white_to_play = true;
  s.passes = 1;
  s.button = -1;
  return s;
}

state no_moves() {
  state s = {0};
  s.visual_area = rectangle(4, 2);
  s.logical_area = single(0, 0) | single(2, 0);
  s.opponent = s.visual_area ^ s.logical_area;
  s.target = s.opponent;
  return s;
}

void test_bulky_five() {
  const state root = bulky_five();
  print_state(&root);
  dual_graph dg = create_dual_graph(&root, COMPRESSED_KEYSPACE);
  for (int i = 0; i < 9; ++i) {
    bool did_change = iterate_dual_graph(&dg, true);
    assert(did_change);
  }
  bool did_change = iterate_dual_graph(&dg, true);
  assert(!did_change);

  state s = root;
  for (int j = 0; j < 4; ++j) {
    float best = -INFINITY;
    state next = s;
    for (int i = 0; i < dg.num_moves; ++i) {
      state child = s;
      move_result r = make_move(&child, dg.moves[i]);
      if (r > TAKE_TARGET) {
        value v = apply_tactics(
          FORCING,
          r,
          &child,
          get_dual_graph_value(&dg, &child, FORCING)
        );
        if (j & 1) {
          if (v.high > best) {
            best = v.high;
            next = child;
          }
        } else if (v.low > best) {
          best = v.low;
          next = child;
        }
      }
    }
    s = next;
  }
  print_state(&s);
  assert(s.button == -1);
  value v = get_dual_graph_value(&dg, &s, NONE);
  assert(v.low == TARGET_CAPTURED_SCORE - BUTTON_BONUS);
  assert(v.high == TARGET_CAPTURED_SCORE - BUTTON_BONUS);

  v = get_dual_graph_value(&dg, &root, FORCING);
  printf("forcing low = %f\n", v.low);
  assert(v.low == TARGET_CAPTURED_SCORE - BUTTON_BONUS - FORCE_BONUS);
  assert(v.high == TARGET_CAPTURED_SCORE - BUTTON_BONUS);

  state terminal = dual_graph_low_terminal(&dg, &root, NONE);
  print_state(&terminal);
  stones_t empty = terminal.visual_area & ~(terminal.player | terminal.opponent);
  assert(root.target & empty);

  // Make sure the rest exist
  dual_graph_high_terminal(&dg, &root, NONE);
  dual_graph_low_terminal(&dg, &root, FORCING);
  dual_graph_high_terminal(&dg, &root, FORCING);

  did_change = area_iterate_dual_graph(&dg, true);
  assert(!did_change);

  free_dual_graph(&dg);
}

void test_bent_four_in_the_corner_is_dead() {
  const state root = bent_four_in_the_corner_is_dead();
  bool did_change;
  value v;
  print_state(&root);
  dual_graph dg = create_dual_graph(&root, COMPRESSED_KEYSPACE);
  for (int i = 0; i < 10; ++i) {
    did_change = iterate_dual_graph(&dg, true);
    assert(did_change);
  }
  did_change = iterate_dual_graph(&dg, true);
  assert(!did_change);

  // White can force a super-ko if Black is required to remove the target stones
  v = get_dual_graph_value(&dg, &root, NONE);
  assert(v.low == -8 + BUTTON_BONUS - KO_THREAT_BONUS);
  assert(v.high == TARGET_CAPTURED_SCORE + BUTTON_BONUS - KO_THREAT_BONUS);

  for (int i = 0; i < 4; ++i) {
    did_change = area_iterate_dual_graph(&dg, true);
    assert(did_change);
  }
  did_change = area_iterate_dual_graph(&dg, true);
  assert(!did_change);

  // The rules say that ko-fights cannot be exploited during scoring. White has no recourse.
  v = get_dual_graph_value(&dg, &root, NONE);
  assert(v.low == TARGET_CAPTURED_SCORE + BUTTON_BONUS - KO_THREAT_BONUS);
  assert(v.high == TARGET_CAPTURED_SCORE + BUTTON_BONUS - KO_THREAT_BONUS);

  free_dual_graph(&dg);
}

void test_bent_four_in_the_might_be_seki() {
  const state root = bent_four_in_the_corner_might_be_seki();
  bool did_change;
  value v;
  print_state(&root);
  dual_graph dg = create_dual_graph(&root, COMPRESSED_KEYSPACE);
  for (int i = 0; i < 10; ++i) {
    did_change = iterate_dual_graph(&dg, true);
    assert(did_change);
  }
  did_change = iterate_dual_graph(&dg, true);
  assert(!did_change);

  // White cannot capture the target stones during normal play as given
  v = get_dual_graph_value(&dg, &root, NONE);
  assert(v.low == 4 + BUTTON_BONUS + KO_THREAT_BONUS);
  assert(v.high == 4 + BUTTON_BONUS + KO_THREAT_BONUS);

  for (int i = 0; i < 3; ++i) {
    did_change = area_iterate_dual_graph(&dg, true);
    assert(did_change);
  }
  did_change = area_iterate_dual_graph(&dg, true);
  assert(!did_change);

  // The rules say that ko-fights cannot be exploited during scoring. Black is dead by definition.
  v = get_dual_graph_value(&dg, &root, NONE);
  assert(v.low == -TARGET_CAPTURED_SCORE + BUTTON_BONUS + KO_THREAT_BONUS);
  assert(v.high == -TARGET_CAPTURED_SCORE + BUTTON_BONUS + KO_THREAT_BONUS);

  free_dual_graph(&dg);
}

void test_dead_three() {
  state s = dead_three();
  state root = s;
  root.passes = 0;
  root.button = 0;
  print_state(&root);

  dual_graph dg = create_dual_graph(&root, COMPRESSED_KEYSPACE);
  while (iterate_dual_graph(&dg, true));
  print_state(&s);
  state child = s;
  const move_result r = make_move(&child, pass());
  print_state(&child);
  assert(r == SECOND_PASS);
  assert(-simple_area_score(&child) > -BIG_SCORE);

  value v = get_dual_graph_value(&dg, &s, NONE);
  printf("%f, %f\n", v.low, v.high);
  assert(v.low > -BIG_SCORE);

  while (area_iterate_dual_graph(&dg, true));

  v = get_dual_graph_area_value(&dg, &s);
  printf("%f, %f\n", v.low, v.high);
  assert(v.low < -BIG_SCORE);
}

void test_no_moves_terminals() {
  state s = no_moves();
  print_state(&s);
  dual_graph dg = create_dual_graph(&s, COMPRESSED_KEYSPACE);
  while (iterate_dual_graph(&dg, true));

  state terminal = dual_graph_low_terminal(&dg, &s, NONE);
  print_state(&terminal);
  assert(terminal.passes == 2);

  terminal = dual_graph_high_terminal(&dg, &s, NONE);
  print_state(&terminal);
  assert(terminal.passes == 2);

  terminal = dual_graph_low_terminal(&dg, &s, FORCING);
  print_state(&terminal);
  assert(terminal.passes == 2);

  terminal = dual_graph_high_terminal(&dg, &s, FORCING);
  print_state(&terminal);
  assert(terminal.passes == 2);

  while (area_iterate_dual_graph(&dg, true));

  terminal = dual_graph_low_terminal(&dg, &s, NONE);
  print_state(&terminal);
  assert(terminal.passes == 2);

  terminal = dual_graph_high_terminal(&dg, &s, NONE);
  print_state(&terminal);
  assert(terminal.passes == 2);

  terminal = dual_graph_low_terminal(&dg, &s, FORCING);
  print_state(&terminal);
  assert(terminal.passes == 2);

  terminal = dual_graph_high_terminal(&dg, &s, FORCING);
  print_state(&terminal);
  assert(terminal.passes == 2);
}

void test_seki() {
  state s = parse_state(" \
        b . 0 B x x x x x \
        b . 0 B x x x x x \
        b . 0 B x x x x x \
        W W B B x x x x x \
  ");
  print_state(&s);

  dual_graph dg = create_dual_graph(&s, COMPRESSED_KEYSPACE);
  while (iterate_dual_graph(&dg, true));

  make_move(&s, single(1, 2));
  print_state(&s);

  value v = get_dual_graph_value(&dg, &s, NONE);
  printf("%f, %f\n", v.low, v.high);
  assert(v.low > -BIG_SCORE);
  assert(v.low < BIG_SCORE);
}

int main() {
  test_bulky_five();
  test_bent_four_in_the_corner_is_dead();
  test_bent_four_in_the_might_be_seki();
  test_dead_three();
  test_no_moves_terminals();
  test_seki();
  return 0;
}
