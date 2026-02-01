#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/tablebase.h"
#include "tinytsumego2/full_solver.h"
#include "tsumego.c"

static const char* CORNER_TSUMEGO[] = {
  "Straight Three",
  "Straight Three Defense",
  "Straight Two",
  "Straight Four",
  "Hat Four",
  "Hat Four Defense",
  "Bent Four in the Corner",
  "Bent Four in the Corner (1 ko threat)",
  "Bent Four in the Corner (1 liberty)",
  "Bent Four in the Corner (2 liberties)",
  "Bent Four in the Corner is Dead",
  "Bent Four in the Corner is Dead (defender has threats)",
  "Bent Four in the Corner is Dead (attacker tenuki)",
  "Rectangle Six in the Corner",
  "Rectangle Six in the Corner (1 physical liberty)",
  "Rectangle Six in the Corner (1 liberty)",
  "Rectangle Six in the Corner (2 liberties)",
  "Rectangle Eight in the Corner",
  "Rectangle Eight in the Corner (defender has threats)",
  "Square Nine in the Corner",
};

const size_t NUM_CORNER_TSUMEGO = sizeof(CORNER_TSUMEGO) / sizeof(char*);

int main(int argc, char * argv[]) {
  if (argc < 2) {
    fprintf(stderr, "No input tablebase file given\n");
    return EXIT_FAILURE;
  }
  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    fprintf(stderr, "Failed to open file\n");
    return EXIT_FAILURE;
  }
  tablebase tb = read_tablebase(f);
  fclose(f);

  void fn(state *s) {
    full_graph fg = create_full_graph(s, false, false);
    expand_full_graph(&fg);
    solve_full_graph(&fg, false);

    for (size_t i = 0; i < fg.num_nodes; ++i) {
      value_range v = get_tablebase_value(&tb, fg.states + i);
      if (!isnan(v.low)) {
        if (v.low != fg.values[i].low || v.high != fg.values[i].high) {
          print_state(fg.states + i);
          printf("%f != %f || %f != %f\n", v.low, fg.values[i].low, v.high, fg.values[i].high);
        }
      }
    }

    free_full_graph(&fg);
  }

  for (size_t i = 0; i < NUM_CORNER_TSUMEGO; ++i) {
    printf("%s\n", CORNER_TSUMEGO[i]);
    tsumego t = get_tsumego(CORNER_TSUMEGO[i]);
    fn(&t.state);
  }

  free_tablebase(&tb);

  return EXIT_SUCCESS;
}
