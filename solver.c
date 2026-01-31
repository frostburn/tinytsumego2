#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/full_solver.h"
#include "tinytsumego2/full_reader.h"

#include "tsumego.c"

#define MAX_DEMONSTRATION (100)

full_graph solve(tsumego t, bool verbose) {
  state root = t.state;

  full_graph fg = create_full_graph(&root);
  expand_full_graph(&fg);

  if (verbose) {
    print_state(&root);
    printf("Solution space size = %zu\n", fg.num_nodes);
  }

  solve_full_graph(&fg, true, verbose);

  value root_value = get_full_graph_value(&fg, &root);
  float low = root_value.low;
  float high = root_value.high;

  if (!verbose) {
    goto cleanup;
  }

  printf("Low = %f, high = %f\n", low, high);

  // Make passing the first option when demonstrating
  for (int i = fg.num_moves - 1; i > 0; --i) {
    fg.moves[i] = fg.moves[i - 1];
  }
  fg.moves[0] = pass();

  bool low_to_play = true;
  #ifdef ATTACKER_PLAYS_HIGH
    low_to_play = false;
  #endif

  coord_f colof = root.wide ? column_of_16 : column_of;
  coord_f rowof = root.wide ? row_of_16 : row_of;

  state s = root;
  for (int n = 0; n < MAX_DEMONSTRATION; ++n) {
    for (int j = 0; j < fg.num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, fg.moves[j]);
      if (r == TAKE_TARGET) {
        printf("%c%c: takes target (%f)\n",  colof(fg.moves[j]), rowof(fg.moves[j]), target_lost_score(&child));
      } else if (r == SECOND_PASS) {
        printf("%c%c: game over (%f)\n",  colof(fg.moves[j]), rowof(fg.moves[j]), score(&child));
      } else if (r != ILLEGAL) {
        const value child_value = get_full_graph_value(&fg, &child);
        printf("%c%c: %f, %f\n",  colof(fg.moves[j]), rowof(fg.moves[j]), child_value.low, child_value.high);
      }
    }

    for (int j = 0; j < fg.num_moves; ++j) {
      state child = s;
      const move_result r = make_move(&child, fg.moves[j]);
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
        const value child_value = get_full_graph_value(&fg, &child);
        printf("%c%c: %f, %f\n",  colof(fg.moves[j]), rowof(fg.moves[j]), child_value.low, child_value.high);

        bool good;
        if (low_to_play) {
          good = (-delay_capture(child_value.high) == low);
        } else {
          good = (-delay_capture(child_value.low) == high);
        }

        if (good) {
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
  }

  cleanup:

  if (root_value.low != t.low_delay || root_value.high != t.high_delay) {
    fprintf(stderr, "%f, %f =! %f, %f\n", root_value.low, root_value.high, t.low_delay, t.high_delay);
  }

  assert(root_value.low == t.low_delay);
  assert(root_value.high == t.high_delay);

  return fg;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      full_graph fg = solve(get_tsumego(TSUMEGO_NAMES[i]), false);
      free_full_graph(&fg);
    }
    return EXIT_SUCCESS;
  } else {
    full_graph fg = solve(get_tsumego(argv[1]), true);
    if (argc >= 3) {
      printf("Saving result to %s\n", argv[2]);
      FILE *f = fopen(argv[2], "wb");
      write_full_graph(&fg, f);
    }
    free_full_graph(&fg);
  }
}
