#include <ctype.h>
#include <stdio.h>
#include "tinytsumego2/compressed_reader.h"

static const char CHECKMARK[] = " \xE2\x9C\x93";

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "No input file given\n");
    return EXIT_FAILURE;
  }
  compressed_graph_reader cgr = load_compressed_graph_reader(argv[1]);

  printf("Solution loaded: %d moves, key-space size = %zu\n\n", cgr.num_moves, cgr.keyspace.size);

  printf("Enter coordinates to navigate the game graph.\n Type 'q' to quit.\n\n");

  state s = cgr.keyspace.keyspace.root;

  int width = s.wide ? WIDTH_16 : WIDTH;
  int height = s.wide ? HEIGHT_16 : HEIGHT;

  coord_f colof = s.wide ? column_of_16 : column_of;
  coord_f rowof = s.wide ? row_of_16 : row_of;

  float sign = -1;

  while (true) {
    print_state(&s);

    value v = get_compressed_graph_reader_value(&cgr, &s);

    for (int j = 0; j < cgr.num_moves; ++j) {
      state child = s;
      stones_t move = cgr.moves[j];
      const move_result r = make_move(&child, move);
      if (r == TAKE_TARGET) {
        printf("%c%c: take target",  colof(move), rowof(move));
        if (v.high == -target_lost_score(&child)) {
          printf(CHECKMARK);
        } else if (v.low == -target_lost_score(&child)) {
          printf(" +");
        }
        printf("\n");
      } else if (r == SECOND_PASS) {
        float child_score = score(&child);
        printf("%c%c: %f (game over)",  colof(move), rowof(move), sign * child_score);
        if (v.high == -child_score) {
          printf(CHECKMARK);
        } else if (v.low == -child_score) {
          printf(" +");
        }
        printf("\n");
      } else if (r != ILLEGAL) {
        value child_value = get_compressed_graph_reader_value(&cgr, &child);
        child_value = apply_tactics(NONE, r, &child, child_value);
        bool low_good = (v.high == child_value.low);
        bool high_good = (v.low == child_value.high);
        if (sign > 0) {
          float temp = child_value.low;
          child_value.low = -child_value.high;
          child_value.high = -temp;
        }
        if (child_value.low == child_value.high) {
          printf("%c%c: %f",  colof(move), rowof(move), child_value.low);
          if (low_good) {
            printf(CHECKMARK);
          }
          printf("\n");
        } else {
          printf("%c%c: (%f, %f)",  colof(move), rowof(move), child_value.low, child_value.high);
          if (low_good) {
            printf(" \u2193");
          }
          if (high_good) {
            printf(" \u2191");
          }
          printf("\n");
        }
      }
    }

    stones_t move = pass();
    while (true) {
      char column = 0;
      char row = 0;

      int num_assigned = scanf("%c%c", &column, &row);

      if (num_assigned < 0) {
        fprintf(stderr, "Read error\n");
        goto cleanup;
      }

      if (row != '\n' && row != EOF) {
        // Flush stdin
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
      }

      if (!num_assigned) {
        printf("Type 'q' to quit...\n");
        continue;
      }

      if (num_assigned < 2) {
        row = 0;
      }

      column = tolower(column);
      row = tolower(row);

      if (column == 'q' && (row == ' ' || row == '\n' || row == EOF || !row)) {
        goto cleanup;
      }
      if (column == 'p' && row == 's') {
        move = pass();
      } else {
        column -= 'a';
        row -= '0';
        if (column < 0 || column >= width) {
          printf("Bad column. Try again...\n");
          continue;
        }
        if (row < 0 || row >= height) {
          printf("Bad row. Try again...\n");
          continue;
        }
        move = s.wide ? single_16(column, row) : single(column, row);
      }
      const move_result r = make_move(&s, move);
      if (r == TAKE_TARGET) {
        printf("Target captured!\n");
        goto cleanup;
      } else if (r == SECOND_PASS) {
        printf("Game over: %f\n", score(&s) * sign);
        goto cleanup;
      } else if (r == ILLEGAL) {
        printf("Illegal move. Try again...\n");
        continue;
      } else {
        if (r == KO_THREAT_AND_RETAKE) {
          printf("Ko threat made and answered...\n");
        }
        break;
      }
    }

    sign = -sign;
  }

  cleanup:
  unload_compressed_graph_reader(&cgr);

  return EXIT_SUCCESS;
}
