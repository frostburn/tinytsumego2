#include <assert.h>
#include <stdio.h>
#include "jkiss/jkiss.h"
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/partial_solver.h"

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

  game_graph gg = create_game_graph(&root, NULL);
  solve_game_graph(&gg, true);
  node_proxy np = get_game_graph_node(&gg, &root);
  printf("%g, %g\n\n", np.low, np.high);

  print_game_graph(&gg);

  assert(np.low == -TARGET_CAPTURED_SCORE + BUTTON_BONUS);
  assert(np.high == -TARGET_CAPTURED_SCORE + BUTTON_BONUS);

  free_game_graph(&gg);
}

void test_straight_three_capture() {
  state root = straight_three();
  print_state(&root);

  game_graph gg = create_game_graph(&root, NULL);
  solve_game_graph(&gg, true);
  node_proxy np = get_game_graph_node(&gg, &root);
  printf("%g, %g\n\n", np.low, np.high);

  print_game_graph(&gg);

  assert(np.low == TARGET_CAPTURED_SCORE - BUTTON_BONUS);
  assert(np.high == TARGET_CAPTURED_SCORE - BUTTON_BONUS);

  free_game_graph(&gg);
}

int main() {
  jkiss_init();
  test_straight_two_loss();
  test_straight_three_capture();
  return EXIT_SUCCESS;
}
