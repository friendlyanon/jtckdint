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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "jtckdint.h"

#ifdef __cplusplus
#  define nil nullptr
#  define cast(T, x) (static_cast<T>(x))
#  define align(x) alignas(x)
#else
#  define nil 0
#  define cast(T, x) ((T)(x))
#  define align(x) _Alignas(x)
#endif

#define TMIN_UINT(T) (cast(T, 0))
#define TMAX_UINT(T) (cast(T, ~cast(T, 0)))

#define TBIT(T) (sizeof(T) * 8 - 1)
#define TMIN_INT(T) (cast(T, cast(ckd_uintmax, 1) << TBIT(T)))
#define TMAX_INT(T) (cast(T, (cast(ckd_uintmax, 1) << TBIT(T)) - 1))

#define XCAT(x, y) x##y
#define CAT(x, y) XCAT(x, y)
#define IF(c) CAT(IF_, c)
#define IF_0(t, ...) __VA_ARGS__
#define IF_1(t, ...) t
#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

#ifdef ckd_have_int128
#  define WITH_128(F) F
typedef ckd_intmax i128;
typedef ckd_uintmax u128;
#else
#  define WITH_128(F) EAT
#endif

/* clang-format off */
#define FOR_TYPES(F) \
  F(u, 8) \
  F(u, 16) \
  F(u, 32) \
  F(u, 64) \
  WITH_128(F)(u, 128) \
  F(i, 8) \
  F(i, 16) \
  F(i, 32) \
  F(i, 64) \
  WITH_128(F)(i, 128)
/* clang-format on */

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
      cast(T, -1), \
      cast(T, -2), \
      cast(T, -3), \
      cast(T, -4), \
      cast(T, -5), \
      cast(T, -6), \
      min(T), \
      cast(T, min(T) + 1), \
      cast(T, min(T) + 2), \
      cast(T, min(T) + 3), \
      cast(T, min(T) + 4), \
      max(T), \
      cast(T, max(T) - 1), \
      cast(T, max(T) - 2), \
      cast(T, max(T) - 3), \
      cast(T, max(T) - 4), \
      cast(T, min(T) / 2), \
      cast(T, min(T) / 2 + 1), \
      cast(T, min(T) / 2 + 2), \
      cast(T, min(T) / 2 + 3), \
      cast(T, min(T) / 2 + 4), \
      cast(T, max(T) / 2), \
      cast(T, max(T) / 2 - 1), \
      cast(T, max(T) / 2 - 2), \
      cast(T, max(T) / 2 - 3), \
      cast(T, max(T) / 2 - 4), \
  };
#define X(S, N) \
  Y(S##N, \
    CAT(TMIN_, IF(SIGNED_##S)(INT, UINT)), \
    CAT(TMAX_, IF(SIGNED_##S)(INT, UINT)))
FOR_TYPES(X)
#undef X
#undef Y

static FILE* reference;
static unsigned char ref;
static size_t size;
align(16) static unsigned char buffer[16];
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

#define STRINGIFY_BUFFER 41

static char c1[STRINGIFY_BUFFER];
static char c2[STRINGIFY_BUFFER];
static char c3[STRINGIFY_BUFFER];
static char c4[STRINGIFY_BUFFER];

static void report_mismatch(bool o1, bool o2, int i1, int i2, int i3, int i4)
{
  int in = i1 < i2 ? i1 : i2;
#define msg \
  "Mismatch @ 0x%lX\n  Actual:   (%c) %s\n  Expected: (%c) %s\n  Types: T =" \
  " %s, U = %s, V = %s\n  Operation: %s(%s, %s)\n  Vector indices: i = %d, " \
  "j = %d\n"
#define args \
  cast(unsigned long, offset), '0' + o1, c1 + in, '0' + o2, c2 + in, t_type, \
      u_type, v_type, op, c3 + i3, c4 + i4, i, j
  assert(fprintf(stderr, msg, args) >= 0);
#undef args
#undef msg
}

#define X(S, N) \
  static char const* str_##S##N = #S #N "_t"; \
  static int stringify_##S##N(void const* x_, char* c) \
  { \
    S##N x = *cast(S##N const*, x_); \
    int p = STRINGIFY_BUFFER; \
    WHEN(SIGNED_##S)(bool const s = x < 0); \
    c[--p] = 0; \
    do { \
      S##N tmp = cast(S##N, x % 10); \
      x /= 10; \
      c[--p] = '0' + cast(char, WHEN(SIGNED_##S)(s ? -tmp :) tmp); \
    } while (x != 0); \
    WHEN(SIGNED_##S)(if (s) c[--p] = '-'); \
    memset(c, ' ', cast(size_t, p)); \
    return p; \
  } \
  static bool mismatch_##S##N(bool o1, S##N z1) \
  { \
    bool o2 = (ref & 0x40) != 0; \
    S##N z2 = cast(S##N, read_##N()); \
    if (o1 == o2 && z1 == z2) { \
      return false; \
    } \
    report_mismatch(o1, \
                    o2, \
                    stringify_##S##N(&z1, c1), \
                    stringify_##S##N(&z2, c2), \
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
  for (;;) {
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
  M(T, U, u8) \
  M(T, U, u16) \
  M(T, U, u32) \
  M(T, U, u64) \
  WITH_128(M)(T, U, u128) \
  M(T, U, i8) \
  M(T, U, i16) \
  M(T, U, i32) \
  M(T, U, i64) \
  WITH_128(M)(T, U, i128)

#define MMM(T) \
  static bool test_##T(void) \
  { \
    bool o = false; \
    t_type = str_##T; \
    MM(T, u8) \
    MM(T, u16) \
    MM(T, u32) \
    MM(T, u64) \
    WITH_128(MM)(T, u128) \
    MM(T, i8) \
    MM(T, i16) \
    MM(T, i32) \
    MM(T, i64) \
    WITH_128(MM)(T, i128) \
    v_ptr = nil; \
    u_ptr = nil; \
    return false; \
  }

MMM(u8)
MMM(u16)
MMM(u32)
MMM(u64)
WITH_128(MMM)(u128)
MMM(i8)
MMM(i16)
MMM(i32)
MMM(i64)
WITH_128(MMM)(i128)
EAT()
/* clang-format on */

bool test_odr(int a, int b);

static char const* get_platform(int x)
{
  if (CHAR_BIT != x + 8) {
    return "unknown";
  }

  if (sizeof(int) + sizeof(long) + sizeof(void*) == x + 12) {
    return "ILP32";
  }

  if (sizeof(long) + sizeof(void*) == x + 16) {
    return "LP64";
  }

  if (sizeof(long long) + sizeof(void*) == x + 16) {
    return "LLP64";
  }

  return "unknown";
}

int main(int argc, char* argv[])
{
  (void)argv;

#ifdef ckd_have_int128
#  define msg "+ [%s] intmax: 128\n"
#else
#  define msg "+ [%s] intmax: 64\n"
#endif
  assert(printf(msg, get_platform(argc < 0)) >= 0);
#undef msg

  if (!test_odr(1, -1)) {
    return 1;
  }

  assert(reference = fopen("test.bin", "rb"));

#define X(S, N) \
  if (test_##S##N()) { \
    return 1; \
  }
  FOR_TYPES(X)
#undef X

#ifndef ckd_have_int128
  for (;;) {
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
