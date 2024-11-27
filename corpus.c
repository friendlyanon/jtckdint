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

#if ((!defined(__SIZEOF_INT128__) || defined(__STRICT_ANSI__)) \
     || !(defined(__GNUC__) && __GNUC__ >= 5 && !defined(__ICC) \
          || defined(__has_builtin) && __has_builtin(__builtin_add_overflow) \
              && __has_builtin(__builtin_sub_overflow) \
              && __has_builtin(__builtin_mul_overflow)))
#  error "Need __int128 and GCC builtins to generate 'test.bin'"
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define TMIN_UINT(T) ((T)(0))
#define TMAX_UINT(T) ((T)(~(T)(0)))

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

#define TBIT(T) (sizeof(T) * 8 - 1)
#define TMIN_INT(T) ((T)((uint128_t)(1) << TBIT(T)))
#define TMAX_INT(T) ((T)(((uint128_t)(1) << TBIT(T)) - 1))

#define XCAT(x, y) x##y
#define CAT(x, y) XCAT(x, y)
#define IF(c) CAT(IF_, c)
#define IF_0(t, f) f
#define IF_1(t, f) t

#define FOR_TYPES(F) \
  F(uint, 8) \
  F(uint, 16) \
  F(uint, 32) \
  F(uint, 64) \
  F(uint, 128) \
  F(int, 8) \
  F(int, 16) \
  F(int, 32) \
  F(int, 64) \
  F(int, 128)

#define SIGNED_int 1
#define SIGNED_uint 0

#define Y(T, min, max) \
  static T const k##T[] = { \
      0, \
      1, \
      2, \
      3, \
      4, \
      5, \
      6, \
      (T)(-1), \
      (T)(-2), \
      (T)(-3), \
      (T)(-4), \
      (T)(-5), \
      (T)(-6), \
      min(T), \
      (T)(min(T) + 1), \
      (T)(min(T) + 2), \
      (T)(min(T) + 3), \
      (T)(min(T) + 4), \
      max(T), \
      (T)(max(T) - 1), \
      (T)(max(T) - 2), \
      (T)(max(T) - 3), \
      (T)(max(T) - 4), \
      (T)(min(T) / 2), \
      (T)(min(T) / 2 + 1), \
      (T)(min(T) / 2 + 2), \
      (T)(min(T) / 2 + 3), \
      (T)(min(T) / 2 + 4), \
      (T)(max(T) / 2), \
      (T)(max(T) / 2 - 1), \
      (T)(max(T) / 2 - 2), \
      (T)(max(T) / 2 - 3), \
      (T)(max(T) / 2 - 4), \
  };
#define X(S, N) \
  Y(S##N##_t, \
    CAT(TMIN_, IF(SIGNED_##S)(INT, UINT)), \
    CAT(TMAX_, IF(SIGNED_##S)(INT, UINT)))
FOR_TYPES(X)
#undef X
#undef Y

static FILE* reference;
static uint8_t buffer[1 + sizeof(uint128_t)];

#define output_next(T, op, is_int128) \
  do { \
    T z = 0; \
    unsigned int count = sizeof(T); \
    unsigned int index = sizeof(buffer); \
    uint8_t o = 0; \
    o = (uint8_t)(__builtin_##op##_overflow(x, y, &z)); \
    while (1) { \
      buffer[--index] = (uint8_t)(z & 0xFF); \
      if (--count == 0) { \
        break; \
      } \
      if (sizeof(T) != 1) { \
        z >>= 8; \
      } \
    } \
    buffer[--index] = \
        (uint8_t)(((is_int128) << 7) | (o << 6) | (uint8_t)sizeof(T)); \
    assert(fwrite(buffer + index, 1, sizeof(buffer) - index, reference) \
           == sizeof(buffer) - index); \
  } while (0)

#define M(T, U, V, I) \
  for (i = 0; i != (int)(sizeof(k##U) / sizeof(k##U[0])); ++i) { \
    U x = k##U[i]; \
    for (j = 0; j != (int)(sizeof(k##V) / sizeof(k##V[0])); ++j) { \
      V y = k##V[j]; \
      output_next(T, add, I); \
      output_next(T, sub, I); \
      output_next(T, mul, I); \
    } \
  }

#define MM(T, U, I) \
  M(T, U, uint8_t, 0 | I) \
  M(T, U, uint16_t, 0 | I) \
  M(T, U, uint32_t, 0 | I) \
  M(T, U, uint64_t, 0 | I) \
  M(T, U, uint128_t, 1 | I) \
  M(T, U, int8_t, 0 | I) \
  M(T, U, int16_t, 0 | I) \
  M(T, U, int32_t, 0 | I) \
  M(T, U, int64_t, 0 | I) \
  M(T, U, int128_t, 1 | I)

#define MMM(T, I) \
  static void output_##T(void) \
  { \
    int i = 0; \
    int j = 0; \
    MM(T, uint8_t, 0 | I) \
    MM(T, uint16_t, 0 | I) \
    MM(T, uint32_t, 0 | I) \
    MM(T, uint64_t, 0 | I) \
    MM(T, uint128_t, 1 | I) \
    MM(T, int8_t, 0 | I) \
    MM(T, int16_t, 0 | I) \
    MM(T, int32_t, 0 | I) \
    MM(T, int64_t, 0 | I) \
    MM(T, int128_t, 1 | I) \
  }

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

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  assert(reference = fopen("test.bin", "wb"));

#define X(S, N) output_##S##N##_t();
  FOR_TYPES(X)
#undef X

  assert(fclose(reference) == 0);
  return 0;
}
