void read_one(void *restrict ptr, size_t size, FILE *restrict stream) {
  if (!fread(ptr, size, 1, stream)) {
    fprintf(stderr, "Failed to read a single item\n");
    exit(EXIT_FAILURE);
  }
}

void read_many(void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream) {
  if (!fread(ptr, size, nitems, stream)) {
    fprintf(stderr, "Failed to read %zu item\n", nitems);
    exit(EXIT_FAILURE);
  }
}
