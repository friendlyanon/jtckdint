/**
 * Copyright 2023 Justine Alexandra Roberts Tunney
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if ((!defined(__SIZEOF_INT128__) || defined(__STRICT_ANSI__)) \
     || !(defined(__GNUC__) && __GNUC__ >= 5 && !defined(__ICC) \
          || defined(__has_builtin) && __has_builtin(__builtin_add_overflow) \
              && __has_builtin(__builtin_sub_overflow) \
              && __has_builtin(__builtin_mul_overflow)))
#  error "Need __int128 and GCC builtins to generate 'test.bin'"
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#ifndef INT64
#  define INT64 long
#endif

typedef char static_assert_char_bit_is_8_bits[(CHAR_BIT == 8) * 2 - 1];
typedef char static_assert_fundamentals_match_stdint
    [(sizeof(short) == 2 && sizeof(int) == 4 && sizeof(INT64) == 8) * 2 - 1];

#define TMIN_U(T) ((T)(0))
#define TMAX_U(T) ((T)(~(T)(0)))

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef INT64 i64;
typedef unsigned INT64 u64;
typedef __int128 i128;
typedef unsigned __int128 u128;

#define TBIT(T) (sizeof(T) * 8 - 1)
#define TMIN_I(T) ((T)((u128)(1) << TBIT(T)))
#define TMAX_I(T) ((T)(((u128)(1) << TBIT(T)) - 1))

#define XCAT(x, y) x##y
#define CAT(x, y) XCAT(x, y)
#define IF(c) CAT(IF_, c)
#define IF_0(t, f) f
#define IF_1(t, f) t

#define FOR_TYPES(F) \
  F(u, 8) \
  F(u, 16) \
  F(u, 32) \
  F(u, 64) \
  F(u, 128) \
  F(i, 8) \
  F(i, 16) \
  F(i, 32) \
  F(i, 64) \
  F(i, 128)

#define SIGNED_i 1
#define SIGNED_u 0

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
  Y(S##N, CAT(TMIN_, IF(SIGNED_##S)(I, U)), CAT(TMAX_, IF(SIGNED_##S)(I, U)))
FOR_TYPES(X)
#undef X
#undef Y

static FILE* reference;
static u8 buffer[1 + sizeof(u128)];

#define output_next(T, op, is_int128) \
  do { \
    T z = 0; \
    unsigned int count = sizeof(T); \
    unsigned int index = sizeof(buffer); \
    unsigned int to_write = 0; \
    u8 o = (u8)(__builtin_##op##_overflow(x, y, &z)); \
    while (1) { \
      buffer[--index] = (u8)(z & 0xFF); \
      if (--count == 0) { \
        break; \
      } \
      z >>= 4; \
      z >>= 4; \
    } \
    buffer[--index] = (u8)(((is_int128) << 7) | (o << 6) | (u8)sizeof(T)); \
    to_write = sizeof(buffer) - index; \
    assert(fwrite(buffer + index, 1, to_write, reference) == to_write); \
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
  M(T, U, u8, 0 | I) \
  M(T, U, u16, 0 | I) \
  M(T, U, u32, 0 | I) \
  M(T, U, u64, 0 | I) \
  M(T, U, u128, 1 | I) \
  M(T, U, i8, 0 | I) \
  M(T, U, i16, 0 | I) \
  M(T, U, i32, 0 | I) \
  M(T, U, i64, 0 | I) \
  M(T, U, i128, 1 | I)

#define MMM(T, I) \
  static void output_##T(void) \
  { \
    int i = 0; \
    int j = 0; \
    MM(T, u8, 0 | I) \
    MM(T, u16, 0 | I) \
    MM(T, u32, 0 | I) \
    MM(T, u64, 0 | I) \
    MM(T, u128, 1 | I) \
    MM(T, i8, 0 | I) \
    MM(T, i16, 0 | I) \
    MM(T, i32, 0 | I) \
    MM(T, i64, 0 | I) \
    MM(T, i128, 1 | I) \
  }

MMM(u8, 0)
MMM(u16, 0)
MMM(u32, 0)
MMM(u64, 0)
MMM(u128, 1)
MMM(i8, 0)
MMM(i16, 0)
MMM(i32, 0)
MMM(i64, 0)
MMM(i128, 1)

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  assert(reference = fopen("test.bin", "wb"));

#define X(S, N) output_##S##N();
  FOR_TYPES(X)
#undef X

  assert(fclose(reference) == 0);
  return 0;
}
