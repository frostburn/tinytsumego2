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

void test_bulky_five() {
  const state root = bulky_five();
  print_state(&root);
  dual_graph dg = create_dual_graph(&root);
  for (int i = 0; i < 12; ++i) {
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
  free_dual_graph(&dg);
}

int main() {
  test_bulky_five();
  return 0;
}
