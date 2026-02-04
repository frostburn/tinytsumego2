#include "tinytsumego2/util.h"

int ceil_div(int x, int y) {
  return (x + y - 1) / y;
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
