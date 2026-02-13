#pragma once

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef unsigned short int value_id_t;

#define VALUE_MAP_SIZE (1 << (sizeof(value_id_t) * CHAR_BIT))

int ceil_div(int x, int y);

size_t ceil_divz(size_t x, size_t y);

char* file_to_mmap(const char *filename, struct stat *sb, int *fd);
