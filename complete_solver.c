#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/complete_solver.h"

#include "tsumego.c"

#define MAX_DEMONSTRATION (100)

complete_graph solve(tsumego t, bool use_delay, bool root_only, bool verbose) {
  state root = t.state;

  complete_graph cg = create_complete_graph(&root, use_delay);

  if (verbose) {
    print_state(&root);
    printf("Solution space size = %zu\n", tight_keyspace_size(&root));
  }

  solve_complete_graph(&cg, root_only, verbose);

  value root_value = get_complete_graph_value(&cg, &root);
  float low = root_value.low;
  float high = root_value.high;

  if (!verbose) {
    goto cleanup;
  }

  printf("Low = %f, high = %f\n", low, high);

  // Make passing the first option when demonstrating
  for (int i = cg.num_moves - 1; i > 0; --i) {
    cg.moves[i] = cg.moves[i - 1];
  }
  cg.moves[0] = pass();

  bool low_to_play = true;
  #ifdef ATTACKER_PLAYS_HIGH
    low_to_play = false;
  #endif

  coord_f colof = root.wide ? column_of_16 : column_of;
  coord_f rowof = root.wide ? row_of_16 : row_of;

  state s = root;
  for (int n = 0; n < MAX_DEMONSTRATION; ++n) {
    bool found = false;
    for (int j = 0; j < cg.num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, cg.moves[j]);
      if (r == TAKE_TARGET) {
        printf("%c%c: takes target (%f)\n",  colof(cg.moves[j]), rowof(cg.moves[j]), target_lost_score(&child));
      } else if (r == SECOND_PASS) {
        printf("%c%c: game over (%f)\n",  colof(cg.moves[j]), rowof(cg.moves[j]), score(&child));
      } else if (r != ILLEGAL) {
        const value child_value = get_complete_graph_value(&cg, &child);
        printf("%c%c: %f, %f\n",  colof(cg.moves[j]), rowof(cg.moves[j]), child_value.low, child_value.high);
      }
    }

    for (int j = 0; j < cg.num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, cg.moves[j]);
      if (r == TAKE_TARGET) {
        print_state(&child);
        printf("Target captured\n");
        goto cleanup;
      } else if (r == SECOND_PASS) {
        float good_score = low_to_play ? low : high;
        if (-score(&child) == good_score) {
          print_state(&child);
          printf("Game over\n");
          goto cleanup;
        } else {
          continue;
        }
      } else if (r != ILLEGAL) {
        const value child_value = get_complete_graph_value(&cg, &child);

        bool good;
        if (low_to_play) {
          good = use_delay ? (-delay_capture(child_value.high) == low) : -child_value.high == low;
        } else {
          good = use_delay ? (-delay_capture(child_value.low) == high) : -child_value.low == high;
        }

        if (good) {
          found = true;
          s = child;
          low_to_play = !low_to_play;
          low = child_value.low;
          high = child_value.high;
          if (r == KO_THREAT_AND_RETAKE) {
            printf("Ko threat made and answered...\n");
          }
          printf("\n");
          print_state(&s);
          break;
        }
      }
    }
    if (!found) {
      fprintf(stderr, "Next move not found\n");
      goto cleanup;
    }
  }

  cleanup:

  if (use_delay) {
    if (root_value.low != t.low_delay || root_value.high != t.high_delay) {
      fprintf(stderr, "%f, %f =! %f, %f\n", root_value.low, root_value.high, t.low_delay, t.high_delay);
    }

    assert(root_value.low == t.low_delay);
    assert(root_value.high == t.high_delay);
  } else {
    if (root_value.low != t.low || root_value.high != t.high) {
      fprintf(stderr, "%f, %f =! %f, %f\n", root_value.low, root_value.high, t.low, t.high);
    }

    assert(root_value.low == t.low);
    assert(root_value.high == t.high);
  }

  return cg;
}

int main(int argc, char *argv[]) {
  int arg_count = argc;
  bool use_delay = true;
  bool root_only = false;
  int c;
  while ((c = getopt(argc, argv, "dr")) != -1) {
    switch (c) {
      case 'd':
        use_delay = false;
        arg_count--;
        break;
      case 'r':
        root_only = true;
        arg_count--;
        break;
      default:
        abort();
    }
  }

  if (arg_count <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      complete_graph cg = solve(get_tsumego(TSUMEGO_NAMES[i]), use_delay, root_only, false);
      free_complete_graph(&cg);
    }
    return EXIT_SUCCESS;
  } else {
    complete_graph cg = solve(get_tsumego(argv[optind]), use_delay, root_only, true);
    // if (arg_count >= 3) {
    //   char *filename = argv[optind + 1];
    //   printf("Saving result to %s\n", filename);
    //   FILE *f = fopen(filename, "wb");
    //   write_complete_graph(&cg, f);
    // }
    free_complete_graph(&cg);
  }
}
