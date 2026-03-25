#pragma once

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @file util.h
 * @brief Small allocation and file-mapping helpers shared across the project.
 */

/** @brief Compact identifier type for entries in serialized value tables. */
typedef unsigned short int value_id_t;

/** @brief Total number of addressable entries for value_id_t. */
#define VALUE_MAP_SIZE (1 << (sizeof(value_id_t) * CHAR_BIT))

/** @brief Write a scalar field and accumulate serialized byte count in `total`. */
#define WRITE_FIELD(total, stream, field) ((total) += fwrite(&(field), sizeof(field), 1, (stream)) * sizeof(field))

/** @brief Write an array and accumulate serialized byte count in `total`. */
#define WRITE_ARRAY(total, stream, ptr, count) ((total) += fwrite((ptr), sizeof(*(ptr)), (count), (stream)) * sizeof(*(ptr)))

/** @brief Copy one scalar field from a mmap cursor and advance the cursor. */
#define READ_FIELD(map, field)              \
  do {                                      \
    memcpy(&(field), (map), sizeof(field)); \
    (map) += sizeof(field);                 \
  } while (0)

/** @brief Map an array field to a mmap cursor and advance the cursor. */
#define MAP_ARRAY_FIELD(map, field, count)   \
  do {                                       \
    const size_t __count = (count);          \
    (field) = (__typeof__(field))(map);      \
    (map) += sizeof(*(field)) * __count;     \
  } while (0)

/** @brief Return the ceiling of `x / y` for signed integers. */
int ceil_div(int x, int y);

/** @brief Return the ceiling of `x / y` for `size_t` values. */
size_t ceil_divz(size_t x, size_t y);

/** @brief Allocate `size` bytes or abort on failure. */
void *xmalloc(size_t size);

/** @brief Allocate and zero-initialize `count * size` bytes or abort on failure. */
void *xcalloc(size_t count, size_t size);

/** @brief Resize an allocation or abort on failure. */
void *xrealloc(void *ptr, size_t size);

/**
 * @brief Memory-map a file for read-only access.
 *
 * @param filename File to map.
 * @param sb Output stat buffer for file metadata.
 * @param fd Output file descriptor used to keep the mapping alive.
 * @return Pointer to the mapped file contents.
 */
char *file_to_mmap(const char *filename, struct stat *sb, int *fd);
