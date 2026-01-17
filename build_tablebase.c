#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/tablebase.h"
#include "tinytsumego2/full_solver.h"

int main() {
  state root = from_corner_tablebase_key(0);
  print_state(&root);

  full_graph fg = create_full_graph(root);
  printf("%zu nodes explored\n", fg.num_nodes);

  solve_full_graph(&fg);
  value v = get_full_graph_value(&fg, &root);
  printf("Root value %g, %g\n", v.low, v.high);

  free_full_graph(&fg);

  return EXIT_SUCCESS;

  /*
  size_t num_legal = 0;
  float delta;
  for (size_t key = 0; key < TABLEBASE_SIZE; ++key) {
    state s = from_corner_tablebase_key(key);
    size_t k = to_corner_tablebase_key(&s, &delta);
    assert(k == key);
    assert(delta == 0);
    if (is_legal(&s)) {
      num_legal++;
      if ((key & 127) == 0) {
        print_state(&s);
      }
    }
  }
  printf("%zu legal states out of %zu enumerated = %g %%\n", num_legal, TABLEBASE_SIZE, num_legal / (double) TABLEBASE_SIZE * 100);
  return EXIT_SUCCESS;
  */
}
