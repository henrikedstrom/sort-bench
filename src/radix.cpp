// radix.cpp: a fast floating-point radix sort demo
//
//   Copyright (C) Herf Consulting LLC 2001.  All Rights Reserved.
//   Use for anything you want, just tell me what you do with it.
//   Code provided "as-is" with no liabilities for anything that goes wrong.
//

#include "radix.h"

#ifndef PREFETCH
#define PREFETCH 0
#endif

#if PREFETCH

#define pfval 64
#define pfval2 128

// x86/x64 with SSE support
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#define pf(x) \
  _mm_prefetch(reinterpret_cast<const char *>((x) + i + pfval), _MM_HINT_T0)
#define pf2(x) \
  _mm_prefetch(reinterpret_cast<const char *>((x) + i + pfval2), _MM_HINT_T0)

// MSVC on ARM64
#elif defined(_M_ARM64)
#include <arm64intrin.h>  // ARM64 intrinsics for MSVC
#define pf(x) __prefetch((x) + i + pfval)
#define pf2(x) __prefetch((x) + i + pfval2)

// GCC or Clang on any platform with builtin prefetch (e.g., Linux/ARM,
// macOS/ARM)
#elif defined(__GNUC__) || defined(__clang__)
#define pf(x) __builtin_prefetch((x) + i + pfval, 0, 3)
#define pf2(x) __builtin_prefetch((x) + i + pfval2, 0, 3)

// Not supported — fallback
#else
#pragma message( \
    "Prefetch requested but not supported on this platform — disabling.")
#undef PREFETCH
#define PREFETCH 0
#define pf(x)
#define pf2(x)
#endif

#else
// Prefetch disabled
#define pf(x)
#define pf2(x)
#endif

// ================================================================================================
// flip a float for sorting
//  finds SIGN of fp number.
//  if it's 1 (negative float), it flips all bits
//  if it's 0 (positive float), it flips the sign only
// ================================================================================================
inline uint32_t FloatFlip(uint32_t f) {
  uint32_t mask = -int32_t(f >> 31) | 0x80000000;
  return f ^ mask;
}

inline void FloatFlipX(uint32_t &f) {
  uint32_t mask = -int32_t(f >> 31) | 0x80000000;
  f ^= mask;
}

// ================================================================================================
// flip a float back (invert FloatFlip)
//  signed was flipped from above, so:
//  if sign is 1 (negative), it flips the sign bit back
//  if sign is 0 (positive), it flips all bits back
// ================================================================================================
inline uint32_t IFloatFlip(uint32_t f) {
  uint32_t mask = ((f >> 31) - 1) | 0x80000000;
  return f ^ mask;
}

// ---- utils for accessing 11-bit quantities
#define _0(x) (x & 0x7FF)
#define _1(x) (x >> 11 & 0x7FF)
#define _2(x) (x >> 22)

// ================================================================================================
// Main radix sort
// ================================================================================================
void RadixSort11(float *farray, float *sorted, uint32_t elements) {
  uint32_t i;
  uint32_t *sort = (uint32_t *)sorted;
  uint32_t *array = (uint32_t *)farray;

  // 3 histograms on the stack:
  const uint32_t kHist = 2048;
  uint32_t b0[kHist * 3];

  uint32_t *b1 = b0 + kHist;
  uint32_t *b2 = b1 + kHist;

  for (i = 0; i < kHist * 3; i++) {
    b0[i] = 0;
  }
  // memset(b0, 0, kHist * 12);

  // 1.  parallel histogramming pass
  //
  for (i = 0; i < elements; i++) {
    pf(array);

    uint32_t fi = FloatFlip((uint32_t &)array[i]);

    b0[_0(fi)]++;
    b1[_1(fi)]++;
    b2[_2(fi)]++;
  }

  // 2.  Sum the histograms -- each histogram entry records the number of values
  // preceding itself.
  {
    uint32_t sum0 = 0, sum1 = 0, sum2 = 0;
    uint32_t tsum;
    for (i = 0; i < kHist; i++) {
      tsum = b0[i] + sum0;
      b0[i] = sum0 - 1;
      sum0 = tsum;

      tsum = b1[i] + sum1;
      b1[i] = sum1 - 1;
      sum1 = tsum;

      tsum = b2[i] + sum2;
      b2[i] = sum2 - 1;
      sum2 = tsum;
    }
  }

  // byte 0: floatflip entire value, read/write histogram, write out flipped
  for (i = 0; i < elements; i++) {
    uint32_t fi = array[i];
    FloatFlipX(fi);
    uint32_t pos = _0(fi);

    pf2(array);
    sort[++b0[pos]] = fi;
  }

  // byte 1: read/write histogram, copy
  //   sorted -> array
  for (i = 0; i < elements; i++) {
    uint32_t si = sort[i];
    uint32_t pos = _1(si);
    pf2(sort);
    array[++b1[pos]] = si;
  }

  // byte 2: read/write histogram, copy & flip out
  //   array -> sorted
  for (i = 0; i < elements; i++) {
    uint32_t ai = array[i];
    uint32_t pos = _2(ai);

    pf2(array);
    sort[++b2[pos]] = IFloatFlip(ai);
  }

  // to write original:
  // memcpy(array, sorted, elements * 4);
}