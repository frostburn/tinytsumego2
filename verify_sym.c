#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/sym_solver.h"

void verify_goban(stones_t visual_area) {
  state root = {0};
  root.visual_area = visual_area;
  root.logical_area = visual_area;
  print_state(&root);

  printf("Symmetric iteration\n");
  sym_graph sg = create_sym_graph(&root);
  while(iterate_sym_graph(&sg, true));

  printf("Dual iteration\n");
  dual_graph dg = create_dual_graph(&root);
  while(iterate_dual_graph(&dg, true));

  for (size_t i = 0; i < dg.keyspace.size; ++i) {
    state s = from_compressed_key(&(dg.keyspace), i);
    value vd = get_dual_graph_value(&dg, &s, NONE);
    value vs = get_sym_graph_value(&sg, &s, NONE);
    assert(vd.low == vs.low);
    assert(vd.high == vs.high);
  }
}

int main() {
  verify_goban(rectangle(3, 3));
  verify_goban(rectangle(3, 4));
  return EXIT_SUCCESS;
}
