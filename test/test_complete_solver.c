#include <assert.h>
#include <stdio.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/complete_solver.h"

state straight_two() {
  state s = {0};
  s.visual_area = rectangle(3, 2);
  s.logical_area = rectangle(2, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.player = s.target;
  return s;
}

state straight_three() {
  state s = {0};
  s.visual_area = rectangle(4, 2);
  s.logical_area = rectangle(3, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.opponent = s.target;
  return s;
}

state straight_two_wide() {
  state s = {0};
  s.visual_area = rectangle_16(3, 2);
  s.logical_area = rectangle_16(2, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.player = s.target;
  s.wide = true;
  return s;
}

void test_straight_two_loss() {
  state root = straight_two();
  print_state(&root);

  complete_graph cg = create_complete_graph(&root, true);
  solve_complete_graph(&cg, false, true);

  value v = get_complete_graph_value(&cg, &root);

  print_complete_graph(&cg);

  printf("%g, %g\n\n", v.low, v.high);

  assert(v.low == -TARGET_CAPTURED_SCORE + DELAY_BONUS + BUTTON_BONUS);
  assert(v.high == -TARGET_CAPTURED_SCORE + DELAY_BONUS + BUTTON_BONUS);

  free_complete_graph(&cg);
}

void test_straight_three_capture() {
  state root = straight_three();
  print_state(&root);

  complete_graph cg = create_complete_graph(&root, true);
  solve_complete_graph(&cg, true, true);

  value v = get_complete_graph_value(&cg, &root);

  printf("%g, %g\n\n", v.low, v.high);

  assert(v.low == TARGET_CAPTURED_SCORE - 3 * DELAY_BONUS - BUTTON_BONUS);
  assert(v.high == TARGET_CAPTURED_SCORE - 3 * DELAY_BONUS - BUTTON_BONUS);

  free_complete_graph(&cg);
}

void test_straight_three_capture_no_delay() {
  state root = straight_three();
  print_state(&root);

  complete_graph cg = create_complete_graph(&root, false);
  solve_complete_graph(&cg, false, true);

  value v = get_complete_graph_value(&cg, &root);

  printf("%g, %g\n\n", v.low, v.high);

  assert(v.low == TARGET_CAPTURED_SCORE - BUTTON_BONUS);
  assert(v.high == TARGET_CAPTURED_SCORE - BUTTON_BONUS);

  free_complete_graph(&cg);
}

void test_straight_two_loss_wide() {
  state root = straight_two_wide();
  print_state(&root);

  complete_graph cg = create_complete_graph(&root, true);
  solve_complete_graph(&cg, false, true);

  for (int i = 0; i < cg.num_moves; ++i) {
    print_stones_16(cg.moves[i]);
  }

  value v = get_complete_graph_value(&cg, &root);

  printf("%g, %g\n\n", v.low, v.high);

  print_complete_graph(&cg);

  assert(v.low == -TARGET_CAPTURED_SCORE + DELAY_BONUS + BUTTON_BONUS);
  assert(v.high == -TARGET_CAPTURED_SCORE + DELAY_BONUS + BUTTON_BONUS);

  free_complete_graph(&cg);
}

int main() {
  test_straight_two_loss();
  test_straight_three_capture();
  test_straight_three_capture_no_delay();
  test_straight_two_loss_wide();
  return EXIT_SUCCESS;
}
