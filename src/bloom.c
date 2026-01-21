#include "tinytsumego2/bloom.h"

void bloom_insert(unsigned char *bloom, stones_t a, stones_t b) {
  bloom[(a >> 3) & BLOOM_MASK] |= 1 << (a & 7);
  a >>= BLOOM_SHIFT;
  bloom[(a >> 3) & BLOOM_MASK] |= 1 << (a & 7);
  a >>= BLOOM_SHIFT;
  bloom[a >> 3] |= 1 << (a & 7);

  bloom[(b >> 3) & BLOOM_MASK] |= 1 << (b & 7);
  b >>= BLOOM_SHIFT;
  bloom[(b >> 3) & BLOOM_MASK] |= 1 << (b & 7);
  b >>= BLOOM_SHIFT;
  bloom[b >> 3] |= 1 << (b & 7);
}

bool bloom_test(const unsigned char *bloom, stones_t a, stones_t b) {
  if (!(bloom[(a >> 3) & BLOOM_MASK] & (1 << (a & 7)))) {
    return false;
  }
  a >>= BLOOM_SHIFT;
  if (!(bloom[(a >> 3) & BLOOM_MASK] & (1 << (a & 7)))) {
    return false;
  }
  a >>= BLOOM_SHIFT;
  if (!(bloom[a >> 3] & (1 << (a & 7)))) {
    return false;
  }

  if (!(bloom[(b >> 3) & BLOOM_MASK] & (1 << (b & 7)))) {
    return false;
  }
  b >>= BLOOM_SHIFT;
  if (!(bloom[(b >> 3) & BLOOM_MASK] & (1 << (b & 7)))) {
    return false;
  }
  b >>= BLOOM_SHIFT;
  return bloom[b >> 3] & (1 << (b & 7));
}
