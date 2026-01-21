#pragma once

#include "tinytsumego2/stones.h"

// Fixed-size bloom filter
#define BLOOM_SIZE (2097152)
#define BLOOM_MASK (2097151)
#define BLOOM_SHIFT (24)

// Add an entry to the bloom filter based on two independent 64 bit hash values
void bloom_insert(unsigned char *bloom, stones_t a, stones_t b);

// Test bloom filter membership `true` indicates likely membership in the set. `false` indicates that the element definitely isn't in the set.
bool bloom_test(const unsigned char *bloom, stones_t a, stones_t b);
