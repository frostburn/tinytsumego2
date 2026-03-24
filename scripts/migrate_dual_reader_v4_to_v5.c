#include "tinytsumego2/dual_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DUAL_READER_V4 (4)
#define DUAL_READER_V5 (5)

static inline size_t frozen_tail_keys_size_v5(size_t tail_size) { return tail_size / 2; }

static unsigned char *read_file(const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    perror("fopen(input)");
    exit(EXIT_FAILURE);
  }
  if (fseek(f, 0, SEEK_END)) {
    perror("fseek");
    exit(EXIT_FAILURE);
  }
  long len = ftell(f);
  if (len < 0) {
    perror("ftell");
    exit(EXIT_FAILURE);
  }
  rewind(f);
  unsigned char *buf = malloc((size_t)len);
  if (!buf) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
    perror("fread");
    exit(EXIT_FAILURE);
  }
  fclose(f);
  *size = (size_t)len;
  return buf;
}

static void write_file(const char *path, const unsigned char *buf, size_t size) {
  FILE *f = fopen(path, "wb");
  if (!f) {
    perror("fopen(output)");
    exit(EXIT_FAILURE);
  }
  if (fwrite(buf, 1, size, f) != size) {
    perror("fwrite");
    exit(EXIT_FAILURE);
  }
  fclose(f);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input-v4.bin> <output-v5.bin>\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t input_size = 0;
  unsigned char *input = read_file(argv[1], &input_size);
  unsigned char *map = input;

  const int version = ((int *)map)[0];
  map += sizeof(int);
  if (version != DUAL_READER_V4) {
    fprintf(stderr, "Expected version %d, found %d\n", DUAL_READER_V4, version);
    return EXIT_FAILURE;
  }

  map += sizeof(keyspace_type);

  const size_t keyspace_size = ((size_t *)map)[0];
  map += 3 * sizeof(size_t);

  map += sizeof(state);

  const size_t num_checkpoints = ((size_t *)map)[0];
  map += sizeof(size_t);
  map += num_checkpoints * sizeof(size_t);

  const size_t uncompressed_size = ((size_t *)map)[0];
  map += sizeof(size_t);
  map += uncompressed_size * sizeof(unsigned char);

  map += sizeof(size_t); // comp->size
  map += sizeof(double);

  const int num_moves = ((int *)map)[0];
  map += sizeof(int);
  map += (size_t)num_moves * sizeof(stones_t);

  map += keyspace_size * sizeof(value_id_t);

  const size_t bulk_map_size = ((size_t *)map)[0];
  map += sizeof(size_t);
  map += bulk_map_size * sizeof(dual_table_value);

  const size_t tail_size = ((size_t *)map)[0];
  map += sizeof(size_t);
  map += tail_size * sizeof(dual_table_value);

  const size_t *old_tail_keys = (const size_t *)map;
  const unsigned char *tail_keys_start = map;
  map += tail_size * sizeof(size_t);

  if ((size_t)(map - input) != input_size) {
    fprintf(stderr, "Input appears malformed: parser stopped at %zu of %zu bytes\n", (size_t)(map - input), input_size);
    return EXIT_FAILURE;
  }

  const size_t new_tail_keys_size = frozen_tail_keys_size_v5(tail_size);
  const size_t prefix_size = (size_t)(tail_keys_start - input);
  const size_t output_size = prefix_size + new_tail_keys_size * sizeof(size_t);

  unsigned char *output = malloc(output_size);
  if (!output) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  memcpy(output, input, prefix_size);
  ((int *)output)[0] = DUAL_READER_V5;

  size_t *new_tail_keys = (size_t *)(output + prefix_size);
  for (size_t i = 0; i < new_tail_keys_size; ++i) {
    new_tail_keys[i] = old_tail_keys[2 * i + 1];
  }

  write_file(argv[2], output, output_size);

  free(output);
  free(input);
  return EXIT_SUCCESS;
}
