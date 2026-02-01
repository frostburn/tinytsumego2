#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/tablebase.h"
#include "tinytsumego2/full_solver.h"

void populate_table(tsumego_table *tt) {
  state root;

  switch (tt->type) {
    case CORNER:
      root = from_corner_tablebase_key(0);
      break;
    case EDGE:
      root = from_edge_tablebase_key(0);
      break;
    default:
      fprintf(stderr, "Unimplemented table type\n");
      exit(EXIT_FAILURE);
  }
  root.button = tt->button;
  root.ko_threats = tt->ko_threats;
  if (root.wide) {
    root.external = cross_16(root.target) & ~(root.target | root.logical_area);
  } else {
    root.external = cross(root.target) & ~(root.target | root.logical_area);
  }
  while (popcount(root.external) > tt->num_external) {
    root.external ^= LAST_STONE >> clz(root.external);
  }
  root.visual_area |= root.external;
  root.logical_area |= root.external;
  root.opponent |= root.external;

  if (tt->opponent_targetted) {
    stones_t temp = root.player;
    root.player = root.opponent;
    root.opponent = temp;
  }

  full_graph fg = create_full_graph(&root, false, false);

  // Many buttonless states cannot be reached naturally
  for (size_t key = 0; key < TABLEBASE_SIZE; ++key) {
    state s;
    switch (tt->type) {
      case CORNER:
        s = from_corner_tablebase_key(key);
        break;
      case EDGE:
        s = from_edge_tablebase_key(key);
        break;
      case CENTER:
        // TODO
        s = (state) {0};
    }
    s.button = tt->button;
    s.ko_threats = tt->ko_threats;
    s.external = root.external;
    s.visual_area |= s.external;
    s.logical_area |= s.external;
    s.opponent |= s.external;
    if (tt->opponent_targetted) {
      stones_t temp = s.player;
      s.player = s.opponent;
      s.opponent = temp;
    }
    if (is_legal(&s)) {
      add_full_graph_state(&fg, &s);
    }
  }

  expand_full_graph(&fg);

  solve_full_graph(&fg, false, false);

  print_state(&root);
  value root_value = get_full_graph_value(&fg, &root);
  printf("Root value: %f, %f\n", root_value.low, root_value.high);

  tt->values = malloc(TABLEBASE_SIZE * sizeof(table_value));

  size_t num_legal = 0;
  for (size_t key = 0; key < TABLEBASE_SIZE; ++key) {
    state s;
    switch (tt->type) {
      case CORNER:
        s = from_corner_tablebase_key(key);
        break;
      case EDGE:
        s = from_edge_tablebase_key(key);
        break;
      case CENTER:
        // TODO
        s = (state) {0};
    }
    s.button = tt->button;
    s.ko_threats = tt->ko_threats;
    s.external = root.external;
    s.visual_area |= s.external;
    s.logical_area |= s.external;
    s.opponent |= s.external;
    if (tt->opponent_targetted) {
      stones_t temp = s.player;
      s.player = s.opponent;
      s.opponent = temp;
    }
    if (is_legal(&s)) {
      num_legal++;
      value v = get_full_graph_value(&fg, &s);
      if (isnan(v.low)) {
        print_state(&s);
        fprintf(stderr, "Failed to solve\n");
        exit(EXIT_FAILURE);
      }

      float baseline = compensated_liberty_score(&s);

      if (fabs(v.low) >= BIG_SCORE) {
        tt->values[key].low = float_to_score_q7(v.low);
      } else {
        tt->values[key].low = float_to_score_q7(v.low - baseline);
      }
      if (fabs(v.high) >= BIG_SCORE) {
        tt->values[key].high = float_to_score_q7(v.high);
      } else {
        tt->values[key].high = float_to_score_q7(v.high - baseline);
      }
    } else {
      tt->values[key].low = INVALID_SCORE_Q7;
      tt->values[key].high = INVALID_SCORE_Q7;
    }
  }
  printf(
    "%zu table entries solved using %zu aux nodes. %g %% legal\n\n",
    num_legal,
    fg.num_nodes,
    num_legal * 100 / ((double) TABLEBASE_SIZE)
  );
  free_full_graph(&fg);
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("Warning: No output file given. Doing a dry-run only\n");
  }
  tablebase tb = {0};

  for (table_type type = CORNER; type <= EDGE; ++type) {
    for (int button = 0; button <= 1; ++button) {
      for (int ko_threats = -1; ko_threats <= 1; ++ko_threats) {
        for (int num_external = 0; num_external <= 2; ++num_external) {
          for (int t = 0; t <= 1; ++t) {
            tb.num_tables++;
            tb.tables = realloc(tb.tables, tb.num_tables * sizeof(tsumego_table));
            tb.tables[tb.num_tables - 1] = (tsumego_table) {
              type,
              button,
              ko_threats,
              num_external,
              !t,
              NULL
            };
          }
        }
      }
    }
  }

  #pragma omp parallel for
  for (int i = 0; i < tb.num_tables; ++i) {
    populate_table(&(tb.tables[i]));
  }

  FILE *f = NULL;
  if (argc > 1) {
    printf("Opening file %s\n", argv[1]);
    f = fopen(argv[1], "wb");
    size_t n = write_tablebase(&tb, f);
    if (n < tb.num_tables * TABLEBASE_SIZE) {
      fprintf(stderr, "Failed to write everything\n");
    }
    printf("Closing file %s\n", argv[1]);
    fclose(f);
  }
  free_tablebase(&tb);

  return EXIT_SUCCESS;
}
