#include "tinytsumego2/util.h"
#include <assert.h>

int main() {
  assert(xmalloc(0) == NULL);
  assert(xcalloc(0, sizeof(int)) == NULL);
  assert(xcalloc(4, 0) == NULL);
  assert(xrealloc(NULL, 0) == NULL);

  void *ptr = xmalloc(sizeof(int));
  assert(ptr != NULL);
  assert(xrealloc(ptr, 0) == NULL);

  ptr = xcalloc(2, sizeof(int));
  assert(ptr != NULL);
  ptr = xrealloc(ptr, 4 * sizeof(int));
  assert(ptr != NULL);
  free(ptr);

  return 0;
}
