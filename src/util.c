#include "tinytsumego2/util.h"

static void allocation_failure(const char *fn, size_t count, size_t size) {
  fprintf(stderr, "%s failed to allocate %zu byte(s)", fn, count * size);
  if (count != 1) {
    fprintf(stderr, " (%zu element(s) of %zu byte(s))", count, size);
  }
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

int ceil_div(int x, int y) {
  return (x + y - 1) / y;
}

size_t ceil_divz(size_t x, size_t y) {
  return (x + y - 1) / y;
}

void* xmalloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  void *ptr = malloc(size);
  if (ptr == NULL) {
    allocation_failure("malloc", 1, size);
  }
  return ptr;
}

void* xcalloc(size_t count, size_t size) {
  if (count == 0 || size == 0) {
    return NULL;
  }

  void *ptr = calloc(count, size);
  if (ptr == NULL) {
    allocation_failure("calloc", count, size);
  }
  return ptr;
}

void* xrealloc(void *ptr, size_t size) {
  if (size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, size);
  if (result == NULL) {
    allocation_failure("realloc", 1, size);
  }
  return result;
}

char* file_to_mmap(const char *filename, struct stat *sb, int *fd) {
  stat(filename, sb);
  *fd = open(filename, O_RDONLY);
  assert(*fd != -1);
  char *map;
  map = (char*) mmap(NULL, sb->st_size, PROT_READ, MAP_SHARED, *fd, 0);
  madvise(map, sb->st_size, MADV_RANDOM);
  return map;
}
