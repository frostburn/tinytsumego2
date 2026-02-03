#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/status.h"
#include "tinytsumego2/shape.h"

#define A_MAX (3)
#define B_MAX (4)

int main(int argc, char *argv[]) {
  int n = 1;
  char x = 'N';
  char y = 'N';
  if (argc > 1) {
    n = atoi(argv[1]);
  }
  if (argc > 2) {
    x = argv[2][0];
  }
  if (argc > 3) {
    y = argv[3][0];
  }

  printf("Computing...\n");
  tsumego_status *tss = malloc(A_MAX * B_MAX * sizeof(tsumego_status));
  #pragma omp parallel for collapse(2)
  for (int a = 1; a <= A_MAX; ++a) {
    for (int b = 1; b <= B_MAX; ++b) {
      if (x == y && b < a) {
        continue;
      }
      char *code = malloc(26 * sizeof(char));
      sprintf(code, "%d%d%d%c%c", n, a, b, x, y);
      state s = notcher(code);
      tsumego_status ts = get_tsumego_status(&s);
      tss[(a - 1) * B_MAX + (b - 1)] = ts;
      printf("Done with %s: %s\n", code, tsumego_status_string(ts));
      free(code);
    }
  }
  if (x == y) {
    for (int a = 1; a <= A_MAX; ++a) {
      for (int b = 1; b < a; ++b) {
        tss[(a - 1) * B_MAX + (b - 1)] = tss[(b - 1) * B_MAX + (a - 1)];
      }
    }
  }
  printf("\nTable of notchers %dab%c%c\n", n, x, y);
  for (int a = 1; a <= A_MAX; ++a) {
    printf("| ");
    for (int b = 1; b <= B_MAX; ++b) {
      printf("%s | ", tsumego_status_string(tss[(a - 1) * B_MAX + (b - 1)]));
    }
    printf("\n");
  }
  free(tss);
  return EXIT_SUCCESS;
}
