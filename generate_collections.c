#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "tinytsumego2/dual_solver.h"

#include "public_tsumego.c"

int main(int argc, char *argv[]) {
  printf("Generating solutions to all public tsumegos...\n");

  size_t num_collections = 0;
  collection *collections = get_collections(&num_collections);

  for (size_t i = 0; i < num_collections; ++i) {
    printf("%s\n", collections[i].title);
    print_state(&(collections[i].root));
    dual_graph dg = create_dual_graph(&(collections[i].root));
    for (int j = 0;;j++) {
      bool verbose = (j < 8) || (j % (j >> 2) == 0);
      if (!iterate_dual_graph(&dg, verbose)) break;
    }
    printf("%zu\n", collections[i].num_tsumegos);
    for (size_t j = 0; j < collections[i].num_tsumegos; ++j) {
      tsumego t = collections[i].tsumegos[j];
      printf(" %s\n", t.subtitle);
      print_state(&(t.state));
      value v = get_dual_graph_value(&dg, &(t.state), NONE);
      printf(" value: %f, %f\n", v.low, v.high);
      if (!isnan(t.value.low)) {
        assert(t.value.low == v.low);
        assert(t.value.high == v.high);
      }
    }
    // TODO: Save the solution to disk
    free_dual_graph(&dg);
  }

  return EXIT_SUCCESS;
}
