#include <assert.h>
#include <math.h>
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

state bulky_five() {
  state s = {0};
  s.visual_area = rectangle(5, 4);
  s.logical_area = (rectangle(2, 2) << (H_SHIFT + V_SHIFT)) | single(3, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.opponent = s.target;
  return s;
}

void test_straight_two_loss() {
  state root = straight_two();
  print_state(&root);

  complete_graph cg = create_complete_graph(&root, DELAY);
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

  complete_graph cg = create_complete_graph(&root, DELAY);
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

  complete_graph cg = create_complete_graph(&root, NONE);
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

  complete_graph cg = create_complete_graph(&root, DELAY);
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

void test_bulky_five_forcing() {
  state root = bulky_five();
  print_state(&root);

  complete_graph cg = create_complete_graph(&root, FORCING);
  solve_complete_graph(&cg, false, true);

  value v = get_complete_graph_value(&cg, &root);
  assert(v.low < v.high);
  assert(v.low > BIG_SCORE);

  state s = root;
  make_move(&s, single(2, 1));
  v = get_complete_graph_value(&cg, &s);
  assert(v.low < v.high);
  assert(v.high < -BIG_SCORE);

  // Make sure that playing the forcing move is unambiguously the best move
  float best = -INFINITY;
  for (int i = 0; i < cg.num_moves; ++i) {
    state child = s;
    const move_result r = make_move(&child, cg.moves[i]);
    if (r != ILLEGAL) {
      print_state(&child);
      v = get_complete_graph_value(&cg, &child);
      printf("%f, %f\n\n", v.low, v.high);
      assert(v.low <= v.high);
      assert(v.low > BIG_SCORE);

      value child_value = apply_tactics(cg.tactics, r, &child, v);
      best = fmax(best, child_value.low);
    }
  }
  for (int i = 0; i < cg.num_moves; ++i) {
    state child = s;
    const move_result r = make_move(&child, cg.moves[i]);
    if (r != ILLEGAL) {
      v = apply_tactics(
        FORCING,
        r,
        &child,
        get_complete_graph_value(&cg, &child)
      );
      if (cg.moves[i] == single(1, 1) || cg.moves[i] == single(2, 2)) {
        assert(v.low == best);
      } else {
        assert(v.low != best);
      }
    }
  }

  make_move(&s, single(1, 1));
  make_move(&s, single(2, 2));

  // No more forcing moves. Make sure tenuki is preferred
  best = -INFINITY;
  for (int i = 0; i < cg.num_moves; ++i) {
    state child = s;
    const move_result r = make_move(&child, cg.moves[i]);
    if (r != ILLEGAL) {
      print_state(&child);
      v = get_complete_graph_value(&cg, &child);
      printf("%f, %f\n\n", v.low, v.high);
      assert(v.low <= v.high);
      assert(v.low > BIG_SCORE);

      value child_value = apply_tactics(cg.tactics, r, &child, v);
      best = fmax(best, child_value.low);
    }
  }
  for (int i = 0; i < cg.num_moves; ++i) {
    state child = s;
    const move_result r = make_move(&child, cg.moves[i]);
    if (r != ILLEGAL) {
      v = apply_tactics(
        FORCING,
        r,
        &child,
        get_complete_graph_value(&cg, &child)
      );
      if (cg.moves[i]) {
        assert(v.low != best);
      } else {
        assert(v.low == best);
        assert(r == TAKE_BUTTON);
        assert(child.button == -1);
      }
    }
  }
}

int main() {
  test_straight_two_loss();
  test_straight_three_capture();
  test_straight_three_capture_no_delay();
  test_straight_two_loss_wide();
  test_bulky_five_forcing();
  return EXIT_SUCCESS;
}
