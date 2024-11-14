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
#include <stdint.h>
#include <stdio.h>

#include "jtckdint.h"

#ifdef __cplusplus
#  define nil nullptr
#  define cast(T, x) (static_cast<T>(x))
#else
#  define nil 0
#  define cast(T, x) ((T)(x))
#  define alignas(x) _Alignas(x)
#endif

#define TBIT(T) (sizeof(T) * 8 - 1)
#define TMIN(T) \
  (cast(T, ~cast(T, 0)) > 1 ? cast(T, 0) \
                            : cast(T, cast(ckd_uintmax, 1) << TBIT(T)))
#define TMAX(T) \
  (cast(T, ~cast(T, 0)) > 1 ? cast(T, ~cast(T, 0)) \
                            : cast(T, (cast(ckd_uintmax, 1) << TBIT(T)) - 1))

#define XCAT(x, y) x##y
#define CAT(x, y) XCAT(x, y)
#define IF(c) CAT(IF_, c)
#define IF_0(t, ...) __VA_ARGS__
#define IF_1(t, ...) t
#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#ifdef ckd_have_int128
#  define WITH_128(F) F
typedef ckd_intmax int128_t;
typedef ckd_uintmax uint128_t;
#else
#  define WITH_128(F) EAT
#endif

/* clang-format off */
#define FOR_TYPES(F) \
  F(uint, 8) \
  F(uint, 16) \
  F(uint, 32) \
  F(uint, 64) \
  WITH_128(F)(uint, 128) \
  F(int, 8) \
  F(int, 16) \
  F(int, 32) \
  F(int, 64) \
  WITH_128(F)(int, 128)
/* clang-format on */

#define Y(T) \
  static const T k##T[] = { \
      0, \
      1, \
      2, \
      3, \
      4, \
      5, \
      6, \
      cast(T, -1), \
      cast(T, -2), \
      cast(T, -3), \
      cast(T, -4), \
      cast(T, -5), \
      cast(T, -6), \
      TMIN(T), \
      cast(T, TMIN(T) + 1), \
      cast(T, TMIN(T) + 2), \
      cast(T, TMIN(T) + 3), \
      cast(T, TMIN(T) + 4), \
      TMAX(T), \
      cast(T, TMAX(T) - 1), \
      cast(T, TMAX(T) - 2), \
      cast(T, TMAX(T) - 3), \
      cast(T, TMAX(T) - 4), \
      cast(T, TMIN(T) / 2), \
      cast(T, TMIN(T) / 2 + 1), \
      cast(T, TMIN(T) / 2 + 2), \
      cast(T, TMIN(T) / 2 + 3), \
      cast(T, TMIN(T) / 2 + 4), \
      cast(T, TMAX(T) / 2), \
      cast(T, TMAX(T) / 2 - 1), \
      cast(T, TMAX(T) / 2 - 2), \
      cast(T, TMAX(T) / 2 - 3), \
      cast(T, TMAX(T) / 2 - 4), \
  };
#define X(S, N) Y(S##N##_t)
FOR_TYPES(X)
#undef X
#undef Y

static FILE* reference;
static unsigned char ref;
static size_t size;
alignas(16) static unsigned char buffer[16];
static long offset;
static int i;
static int j;
static char const* t_type;
static char const* u_type;
static char const* v_type;
static char const* op;
static void const* u_ptr;
static int (*u_stringify)(void const*, char*);
static void const* v_ptr;
static int (*v_stringify)(void const*, char*);

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#ifdef ckd_have_int128
typedef uint128_t u128;
#endif

#define read_8() (cast(u8, buffer[0]))
#define read_16() \
  (cast(u16, (cast(u16, buffer[0]) << 8) | cast(u16, buffer[1])))
#define read_32_(b) \
  (cast(u32, \
        (cast(u32, buffer[(b) * 4]) << 24) \
            | (cast(u32, buffer[(b) * 4 + 1]) << 16) \
            | (cast(u32, buffer[(b) * 4 + 2]) << 8) \
            | cast(u32, buffer[(b) * 4 + 3])))
#define read_32() read_32_(0)
#define read_64_(b) \
  (cast(u64, \
        (cast(u64, read_32_((b) * 2)) << 32) \
            | cast(u64, read_32_((b) * 2 + 1))))
#define read_64() read_64_(0)
#ifdef ckd_have_int128
#  define read_128() \
    (cast(u128, (cast(u128, read_64_(0)) << 64) | cast(u128, read_64_(1))))
#endif

#define STRINGIFY_BUFFER 50

static char c1[STRINGIFY_BUFFER];
static char c2[STRINGIFY_BUFFER];
static char c3[STRINGIFY_BUFFER];
static char c4[STRINGIFY_BUFFER];

static void report_mismatch(bool o1, bool o2, int i1, int i2, int i3, int i4)
{
#define msg \
  "Mismatch @ 0x%lX\n  Actual: (%c) %s\n  Expected: (%c) %s\n  Types: T = " \
  "%s, U = %s, V = %s\n  Operation: %s(%s, %s)\n  Vector indices: i = %d, " \
  "j = %d\n"
#define args \
  cast(unsigned long, offset), '0' + o1, c1 + i1, '0' + o2, c2 + i2, t_type, \
      u_type, v_type, op, c3 + i3, c4 + i4, i, j
  assert(fprintf(stderr, msg, args) >= 0);
#undef args
#undef msg
}

#define SIGNED_int 1
#define SIGNED_uint 0

#define X(S, N) \
  static char const* str_##S##N##_t = #S #N "_t"; \
  static int stringify_##S##N##_t(void const* x_, char* c) \
  { \
    S##N##_t x = *cast(S##N##_t const*, x_); \
    int p = STRINGIFY_BUFFER; \
    WHEN(SIGNED_##S)(bool const s = x < 0); \
    c[--p] = 0; \
    do { \
      S##N##_t tmp = cast(S##N##_t, x % 10); \
      c[--p] = '0' + cast(char, WHEN(SIGNED_##S)(s ? -tmp :) tmp); \
      x /= 10; \
    } while (x != 0); \
    WHEN(SIGNED_##S)(if (s) c[--p] = '-'); \
    return p; \
  } \
  static bool mismatch_##S##N##_t(bool o1, S##N##_t z1) \
  { \
    bool o2 = (ref & 0x40) != 0; \
    S##N##_t z2 = cast(S##N##_t, read_##N()); \
    if (o1 == o2 && z1 == z2) { \
      return false; \
    } \
    report_mismatch(o1, \
                    o2, \
                    stringify_##S##N##_t(&z1, c1), \
                    stringify_##S##N##_t(&z2, c2), \
                    u_stringify(u_ptr, c3), \
                    v_stringify(v_ptr, c4)); \
    return true; \
  }
FOR_TYPES(X)
#undef X

static void read_next(void)
{
  offset = ftell(reference);
#ifdef ckd_have_int128
  assert(fread(&ref, 1, 1, reference) == 1);
  size = cast(size_t, ref & 0x3F);
  assert(fread(buffer, 1, size, reference) == size);
#else
  while (1) {
    assert(fread(&ref, 1, 1, reference) == 1);
    size = cast(size_t, ref & 0x3F);
    if ((ref & 0x80) == 0) {
      assert(fread(buffer, 1, size, reference) == size);
      return;
    }
    assert(fseek(reference, cast(long, size), SEEK_CUR) == 0);
    offset += 1 + cast(long, size);
  }
#endif
}

static char const* str_ckd_add = "ckd_add";
static char const* str_ckd_sub = "ckd_sub";
static char const* str_ckd_mul = "ckd_mul";

#define check_next(T, f) \
  do { \
    op = str_##f; \
    read_next(); \
    o = f(&z, x, y); \
    if (mismatch_##T(o, z)) { \
      return true; \
    } \
  } while (0)

#define M(T, U, V) \
  v_type = str_##V; \
  for (i = 0; i != cast(int, sizeof(k##U) / sizeof(k##U[0])); ++i) { \
    U x = k##U[i]; \
    u_ptr = &x; \
    u_stringify = stringify_##U; \
    for (j = 0; j != cast(int, sizeof(k##V) / sizeof(k##V[0])); ++j) { \
      T z; \
      V y = k##V[j]; \
      v_ptr = &y; \
      v_stringify = stringify_##V; \
      check_next(T, ckd_add); \
      check_next(T, ckd_sub); \
      check_next(T, ckd_mul); \
    } \
  }

/* clang-format off */
#define MM(T, U) \
  u_type = str_##U; \
  M(T, U, uint8_t) \
  M(T, U, uint16_t) \
  M(T, U, uint32_t) \
  M(T, U, uint64_t) \
  WITH_128(M)(T, U, uint128_t) \
  M(T, U, int8_t) \
  M(T, U, int16_t) \
  M(T, U, int32_t) \
  M(T, U, int64_t) \
  WITH_128(M)(T, U, int128_t)

#define MMM(T) \
  static bool test_##T(void) \
  { \
    bool o = false; \
    t_type = str_##T; \
    MM(T, uint8_t) \
    MM(T, uint16_t) \
    MM(T, uint32_t) \
    MM(T, uint64_t) \
    WITH_128(MM)(T, uint128_t) \
    MM(T, int8_t) \
    MM(T, int16_t) \
    MM(T, int32_t) \
    MM(T, int64_t) \
    WITH_128(MM)(T, int128_t) \
    v_ptr = nil; \
    u_ptr = nil; \
    return false; \
  }

MMM(uint8_t)
MMM(uint16_t)
MMM(uint32_t)
MMM(uint64_t)
WITH_128(MMM)(uint128_t)
MMM(int8_t)
MMM(int16_t)
MMM(int32_t)
MMM(int64_t)
WITH_128(MMM)(int128_t)
EAT()
/* clang-format on */

bool test_odr(int a, int b);

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  if (!test_odr(1, -1)) {
    return 1;
  }

  assert(reference = fopen("test.bin", "rb"));

#define X(S, N) \
  if (test_##S##N##_t()) { \
    return 1; \
  }
  FOR_TYPES(X)
#undef X

#ifndef ckd_have_int128
  while (1) {
    if (fread(&ref, 1, 1, reference) != 1) {
      break;
    }
    assert((ref & 0x80) != 0);
    assert(fseek(reference, ref & 0x3F, SEEK_CUR) == 0);
  }
#endif

  if (fgetc(reference) != EOF || !feof(reference)) {
    long current = ftell(reference);
    long end = 0;
    assert(fseek(reference, 0, SEEK_END) == 0);
    end = ftell(reference);
#define msg "Reference was not read to completion. %ld bytes left.\n"
    assert(fprintf(stderr, msg, end - current) >= 0);
#undef msg
    return 1;
  }

  assert(fclose(reference) == 0);
  return 0;
}
