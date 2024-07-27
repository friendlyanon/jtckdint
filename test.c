// Copyright 2023 Justine Alexandra Roberts Tunney
//
// Permission to use, copy, modify, and/or distribute this software for
// any purpose with or without fee is hereby granted, provided that the
// above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
// DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
// PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
// TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "jtckdint.h"

#define TBIT(T) (sizeof(T) * 8 - 1)
#define TMIN(T) (((T) ~(T)0) > 1 ? (T)0 : (T)((ckd_uintmax_t)1 << TBIT(T)))
#define TMAX(T) \
  (((T) ~(T)0) > 1 ? (T) ~(T)0 : (T)(((ckd_uintmax_t)1 << TBIT(T)) - 1))

#ifdef ckd_have_int128
typedef signed __int128 int128_t;
typedef unsigned __int128 uint128_t;
#endif

#define DECLARE_TEST_VECTORS(T) \
  static const T k##T[] = { \
      0, \
      1, \
      2, \
      3, \
      4, \
      5, \
      6, \
      (T) - 1, \
      (T) - 2, \
      (T) - 3, \
      (T) - 4, \
      (T) - 5, \
      (T) - 6, \
      TMIN(T), \
      (T)(TMIN(T) + 1), \
      (T)(TMIN(T) + 2), \
      (T)(TMIN(T) + 3), \
      (T)(TMIN(T) + 4), \
      TMAX(T), \
      (T)(TMAX(T) - 1), \
      (T)(TMAX(T) - 2), \
      (T)(TMAX(T) - 3), \
      (T)(TMAX(T) - 4), \
      (T)(TMIN(T) / 2), \
      (T)(TMIN(T) / 2 + 1), \
      (T)(TMIN(T) / 2 + 2), \
      (T)(TMIN(T) / 2 + 3), \
      (T)(TMIN(T) / 2 + 4), \
      (T)(TMAX(T) / 2), \
      (T)(TMAX(T) / 2 - 1), \
      (T)(TMAX(T) / 2 - 2), \
      (T)(TMAX(T) / 2 - 3), \
      (T)(TMAX(T) / 2 - 4), \
  }

DECLARE_TEST_VECTORS(int8_t);
DECLARE_TEST_VECTORS(uint8_t);
DECLARE_TEST_VECTORS(int16_t);
DECLARE_TEST_VECTORS(uint16_t);
DECLARE_TEST_VECTORS(int32_t);
DECLARE_TEST_VECTORS(uint32_t);
DECLARE_TEST_VECTORS(int64_t);
DECLARE_TEST_VECTORS(uint64_t);
#ifdef ckd_have_int128
DECLARE_TEST_VECTORS(int128_t);
DECLARE_TEST_VECTORS(uint128_t);
#endif

static FILE* reference;

#define print(T, I, o, x) \
  do { \
    T v = (x); \
    int count = sizeof(T); \
    unsigned char buffer[17]; \
    int index = sizeof(buffer); \
    while (1) { \
      buffer[--index] = (unsigned char)(v & 0xFF); \
      if (--count == 0) { \
        break; \
      } \
      v >>= 8; \
    } \
    buffer[--index] = (unsigned char)(((I) << 7) | ((o) << 6) | sizeof(T)); \
    assert(fwrite(buffer + index, 1, sizeof(buffer) - index, reference) \
           == sizeof(buffer) - index); \
  } while (0)

int main(int argc, char const* argv[])
{
  (void)argc;
  (void)argv;

  reference = fopen("test.bin", "wb");
  assert(reference);

#define M(T, U, V, I) \
  for (int i = 0; i != (int)(sizeof(k##U) / sizeof(k##U[0])); ++i) { \
    U x = k##U[i]; \
    for (int j = 0; j != (int)(sizeof(k##V) / sizeof(k##V[0])); ++j) { \
      int o1, o2; \
      T z1, z2; \
      V y = k##V[j]; \
      o1 = ckd_add(&z1, x, y); \
      o2 = __builtin_add_overflow(x, y, &z2); \
      assert(o1 == o2); \
      assert(z1 == z2); \
      print(T, (I), o2, z2); \
      o1 = ckd_sub(&z1, x, y); \
      o2 = __builtin_sub_overflow(x, y, &z2); \
      assert(o1 == o2); \
      assert(z1 == z2); \
      print(T, (I), o2, z2); \
      o1 = ckd_mul(&z1, x, y); \
      o2 = __builtin_mul_overflow(x, y, &z2); \
      assert(o1 == o2); \
      assert(z1 == z2); \
      print(T, (I), o2, z2); \
    } \
  }

#ifdef ckd_have_int128
#  define MM(T, U, I) \
    M(T, U, uint8_t, 0 | (I)) \
    M(T, U, uint16_t, 0 | (I)) \
    M(T, U, uint32_t, 0 | (I)) \
    M(T, U, uint64_t, 0 | (I)) \
    M(T, U, uint128_t, 1 | (I)) \
    M(T, U, int8_t, 0 | (I)) \
    M(T, U, int16_t, 0 | (I)) \
    M(T, U, int32_t, 0 | (I)) \
    M(T, U, int64_t, 0 | (I)) \
    M(T, U, int128_t, 1 | (I))
#  define MMM(T, I) \
    MM(T, uint8_t, 0 | (I)) \
    MM(T, uint16_t, 0 | (I)) \
    MM(T, uint32_t, 0 | (I)) \
    MM(T, uint64_t, 0 | (I)) \
    MM(T, uint128_t, 1 | (I)) \
    MM(T, int8_t, 0 | (I)) \
    MM(T, int16_t, 0 | (I)) \
    MM(T, int32_t, 0 | (I)) \
    MM(T, int64_t, 0 | (I)) \
    MM(T, int128_t, 1 | (I))
  MMM(uint8_t, 0)
  MMM(uint16_t, 0)
  MMM(uint32_t, 0)
  MMM(uint64_t, 0)
  MMM(uint128_t, 1)
  MMM(int8_t, 0)
  MMM(int16_t, 0)
  MMM(int32_t, 0)
  MMM(int64_t, 0)
  MMM(int128_t, 1)

#else
#  define MM(T, U) \
    M(T, U, uint8_t, 0) \
    M(T, U, uint16_t, 0) \
    M(T, U, uint32_t, 0) \
    M(T, U, uint64_t, 0) \
    M(T, U, int8_t, 0) \
    M(T, U, int16_t, 0) \
    M(T, U, int32_t, 0) \
    M(T, U, int64_t, 0)
#  define MMM(T) \
    MM(T, uint8_t) \
    MM(T, uint16_t) \
    MM(T, uint32_t) \
    MM(T, uint64_t) \
    MM(T, int8_t) \
    MM(T, int16_t) \
    MM(T, int32_t) \
    MM(T, int64_t)
  MMM(uint8_t)
  MMM(uint16_t)
  MMM(uint32_t)
  MMM(uint64_t)
  MMM(int8_t)
  MMM(int16_t)
  MMM(int32_t)
  MMM(int64_t)
#endif

  assert(fclose(reference) == 0);
}
