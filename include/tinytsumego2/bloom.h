#pragma once

#include "tinytsumego2/stones.h"

/**
 * @file bloom.h
 * @brief Fixed-size bloom filter helpers for bitboard hashes.
 */

/** @brief Number of bytes in the bloom filter storage. */
#define BLOOM_SIZE (2097152)
/** @brief Mask used to wrap bloom hash indices. */
#define BLOOM_MASK (2097151)
/** @brief Bit shift used to derive an independent bloom hash. */
#define BLOOM_SHIFT (24)

/**
 * @brief Insert an entry into a bloom filter.
 *
 * @param bloom Byte array of size BLOOM_SIZE.
 * @param a First 64-bit hash value.
 * @param b Second 64-bit hash value.
 */
void bloom_insert(unsigned char *bloom, stones_t a, stones_t b);

/**
 * @brief Test whether an entry may exist in a bloom filter.
 *
 * @param bloom Byte array of size BLOOM_SIZE.
 * @param a First 64-bit hash value.
 * @param b Second 64-bit hash value.
 * @return False when the entry is definitely absent, true when it may exist.
 */
bool bloom_test(const unsigned char *bloom, stones_t a, stones_t b);
