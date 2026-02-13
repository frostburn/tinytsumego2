#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/dual_reader.h"

#include "tsumego.c"

dual_graph solve(tsumego t, bool verbose) {
  state root = t.state;

  dual_graph dg = create_dual_graph(&root);

  if (verbose) {
    print_state(&root);
    printf("Solution space size = %zu\n", dg.keyspace.size);
  }

  while(iterate_dual_graph(&dg, verbose));

  value root_value = get_dual_graph_value(&dg, &root, NONE);

  if (verbose)
    printf("Low = %f, high = %f\n", root_value.low, root_value.high);

  if (root_value.low != t.low || root_value.high != t.high) {
    fprintf(stderr, "%f, %f =! %f, %f\n", root_value.low, root_value.high, t.low, t.high);
  }

  assert(root_value.low == t.low);
  assert(root_value.high == t.high);

  root_value = get_dual_graph_value(&dg, &root, FORCING);
  if (verbose)
    printf("Root value (forcing) = (%f, %f)\n\n", root_value.low, root_value.high);

  return dg;
}

int main(int argc, char *argv[]) {
  int arg_count = argc;

  if (arg_count <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      dual_graph dg = solve(get_tsumego(TSUMEGO_NAMES[i]), false);
      free_dual_graph(&dg);
    }
    return EXIT_SUCCESS;
  } else {
    dual_graph dg = solve(get_tsumego(argv[1]), true);
    if (arg_count >= 3) {
      char *filename = argv[2];
      printf("Saving result to %s\n", filename);
      FILE *f = fopen(filename, "wb");
      write_dual_graph(&dg, f);
      fclose(f);
    }
    free_dual_graph(&dg);
  }
}
