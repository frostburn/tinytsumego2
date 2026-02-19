#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/compressed_graph.h"
#include "tinytsumego2/compressed_reader.h"

#include "tsumego.c"

compressed_graph solve(tsumego t, bool verbose) {
  state root = t.state;

  compressed_graph cg = create_compressed_graph(&root);

  if (verbose) {
    print_state(&root);
    printf("Solution space size = %zu\n", cg.keyspace.size);
  }

  while(iterate_compressed_graph(&cg, verbose));

  value root_value = get_compressed_graph_value(&cg, &root);

  if (verbose)
    printf("Low = %f, high = %f\n", root_value.low, root_value.high);

  if (root_value.low != t.low || root_value.high != t.high) {
    fprintf(stderr, "%f, %f =! %f, %f\n", root_value.low, root_value.high, t.low, t.high);
  }

  assert(root_value.low == t.low);
  assert(root_value.high == t.high);

  return cg;
}

int main(int argc, char *argv[]) {
  int arg_count = argc;

  if (arg_count <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      compressed_graph cg = solve(get_tsumego(TSUMEGO_NAMES[i]), false);
      free_compressed_graph(&cg);
    }
    return EXIT_SUCCESS;
  } else {
    compressed_graph cg = solve(get_tsumego(argv[1]), true);
    if (arg_count >= 3) {
      char *filename = argv[2];
      printf("Saving result to %s\n", filename);
      FILE *f = fopen(filename, "wb");
      write_compressed_graph(&cg, f);
      fclose(f);
    }
    free_compressed_graph(&cg);
  }
}
