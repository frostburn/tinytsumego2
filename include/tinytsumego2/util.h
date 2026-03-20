#pragma once

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef unsigned short int value_id_t;

#define VALUE_MAP_SIZE (1 << (sizeof(value_id_t) * CHAR_BIT))

int ceil_div(int x, int y);

size_t ceil_divz(size_t x, size_t y);

void* xmalloc(size_t size);

void* xcalloc(size_t count, size_t size);

void* xrealloc(void *ptr, size_t size);

char* file_to_mmap(const char *filename, struct stat *sb, int *fd);
