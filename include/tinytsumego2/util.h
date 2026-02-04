#pragma once

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

int ceil_div(int x, int y);

char* file_to_mmap(const char *filename, struct stat *sb, int *fd);
