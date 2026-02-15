#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinytsumego2/collection.h"
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/dual_reader.h"

int main(int argc, char *argv[]) {
  printf("Generating solutions to all public tsumegos...\n");

  char *path = NULL;
  if (argc < 2) {
    fprintf(stderr, "No output folder provided. Doing a dry-run\n");
  } else {
    path = argv[1];
    if (path[strlen(path) - 1] != '/') {
      // XXX: Leaks a tiny bit of memory
      path = malloc((strlen(argv[1]) + 2) * sizeof(char));
      memcpy(path, argv[1], strlen(argv[1]) * sizeof(char));
      path[strlen(argv[1])] = '/';
      path[strlen(argv[1]) + 1] = 0;
    }
    printf("Saving under %s\n", path);
  }

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
    printf("%zu tsumegos in collection\n", collections[i].num_tsumegos);
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
    if (path) {
      char *filename = malloc((strlen(path) + strlen(collections[i].slug) + strlen(".bin") + 1) * sizeof(char));
      sprintf(filename, "%s%s.bin", path, collections[i].slug);
      printf("Storing solution to %s\n", filename);
      FILE *f = fopen(filename, "wb");
      write_dual_graph(&dg, f);
      fclose(f);
      free(filename);
      free_dual_graph(&dg);
      free(collections[i].tsumegos);
    }
  }
  free(collections);

  return EXIT_SUCCESS;
}
