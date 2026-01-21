#include <assert.h>
#include <stdio.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/full_solver.h"

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

void test_straight_two_loss() {
  state root = straight_two();
  print_state(&root);

  full_graph fg = create_full_graph(&root);
  expand_full_graph(&fg);
  solve_full_graph(&fg);

  value v = get_full_graph_value(&fg, &root);

  printf("%g, %g\n\n", v.low, v.high);

  assert(v.low == -TARGET_CAPTURED_SCORE + 0.5 + 0.25);
  assert(v.high == -TARGET_CAPTURED_SCORE + 0.5 + 0.25);

  free_full_graph(&fg);
}

void test_straight_three_capture() {
  state root = straight_three();
  print_state(&root);

  full_graph fg = create_full_graph(&root);
  expand_full_graph(&fg);
  solve_full_graph(&fg);

  value v = get_full_graph_value(&fg, &root);

  printf("%g, %g\n\n", v.low, v.high);

  assert(v.low == TARGET_CAPTURED_SCORE - 0.5 - 3 * 0.25);
  assert(v.high == TARGET_CAPTURED_SCORE - 0.5 - 3 * 0.25);

  free_full_graph(&fg);
}

int main() {
  test_straight_two_loss();
  test_straight_three_capture();
  return EXIT_SUCCESS;
}
