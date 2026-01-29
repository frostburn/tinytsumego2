#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "jkiss/jkiss.h"
#include "jkiss/util.h"
#include "tinytsumego2/partial_solver.h"
#include "tinytsumego2/scoring.h"

#include "tsumego.c"

int solve(tsumego t, bool verbose, tablebase *tb) {
  state root = t.state;
  if (verbose) {
    print_state(&root);
  }

  game_graph gg = create_game_graph(&root, tb);

  solve_game_graph(&gg, verbose);
  size_t root_key = to_key(&root, &root);
  node_proxy np = get_game_graph_node(&gg, root_key, &root);

  if (verbose) {
    printf("%zu nodes expanded\n", gg.num_nodes);

    printf(
      "%f%s, %f%s\n",
      np.low, np.low_fixed ? "*" : "",
      np.high, np.high_fixed ? "*" : ""
    );
  }

  #ifdef DEBUG_SEARCH
  for (int j = 1; j < 10; ++j) {
    printf("\nDepth %d\n", j);
    for (size_t i = 0; i < num_nodes; ++i) {
      if (nodes[i].depth == j) {
        printf(" %zu: %f%s, %f%s -> ", i, nodes[i].low, nodes[i].low_fixed ? "*" : "", nodes[i].high, nodes[i].high_fixed ? "*" : "");
        for (int k = 0; k < num_moves; ++k) {
          state child = nodes[i].state;
          move_result r = make_move(&child, moves[k]);
          // TODO: Apply Benson's
          if (r == SECOND_PASS) {
            printf("sp = %f, ", score(&child));
          } else if (r == TAKE_TARGET) {
            printf("tt = %f, ", target_lost_score(&child));
          } else if (r != ILLEGAL) {
            for (size_t l = 0; l < num_nodes; ++l) {
              if (equals(&child, &(nodes[l].state))) {
                printf("%zu, ", l);
              }
            }
          }
        }
        printf("\n");
      }
    }
  }
  #endif

  float low = np.low;
  float high = np.high;

  free_game_graph(&gg);

  printf("%f, %f\n", low, high);

  assert(low == t.low);
  assert(high == t.high);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  jkiss_init();

  FILE *tablebase_file = NULL;

  int c;
  while ((c = getopt(argc, argv, "t::")) != -1) {
    switch (c) {
      case 't':
        printf("Opening tablebase file %s\n", optarg);
        tablebase_file = fopen(optarg, "rb");
        if (!tablebase_file) {
          fprintf(stderr, "Failed to open file\n");
          return EXIT_FAILURE;
        }
        break;
      default:
        abort();
    }
  }

  tablebase tb = {0};
  tablebase *tb_ptr = NULL;
  if (tablebase_file) {
    tb = read_tablebase(tablebase_file);
    tb_ptr = &tb;
    fclose(tablebase_file);
  }

  if (optind >= argc) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      solve(get_tsumego(TSUMEGO_NAMES[i]), true, tb_ptr);
    }
    return EXIT_SUCCESS;
  } else {
    return solve(get_tsumego(argv[optind]), true, tb_ptr);
  }

  if (tb_ptr) {
    free_tablebase(tb_ptr);
  }
  return EXIT_SUCCESS;
}
