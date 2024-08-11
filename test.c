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
#define TMIN(T) (((T) ~(T)0) > 1 ? (T)0 : (T)((ckd_uintmax)1 << TBIT(T)))
#define TMAX(T) \
  (((T) ~(T)0) > 1 ? (T) ~(T)0 : (T)(((ckd_uintmax)1 << TBIT(T)) - 1))

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

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4293)
#endif
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
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

static FILE* reference;
static unsigned char ref;
static size_t size;
static _Alignas(16) unsigned char buffer[16];
static long offset;
static int i;
static int j;
static char const* t_type;
static char const* u_type;
static char const* v_type;
static char const* op;
static void* u_ptr;
static int (*u_stringify)(void const*, char*);
static void* v_ptr;
static int (*v_stringify)(void const*, char*);

typedef uint8_t tmp_8;
typedef uint16_t tmp_16;
typedef uint32_t tmp_32;
typedef uint64_t tmp_64;
#ifdef ckd_have_int128
typedef uint128_t tmp_128;
#endif

#define read_8() ((tmp_8)buffer[0])
#define read_16() (((tmp_16)buffer[0] << 8) | (tmp_16)buffer[1])
#define read_32_(b) \
  (((tmp_32)buffer[(b) * 4] << 24) | ((tmp_32)buffer[(b) * 4 + 1] << 16) \
   | ((tmp_32)buffer[(b) * 4 + 2] << 8) | (tmp_32)buffer[(b) * 4 + 3])
#define read_32() read_32_(0)
#define read_64_(b) \
  (((tmp_64)read_32_((b) * 2) << 32) | (tmp_64)read_32_((b) * 2 + 1))
#define read_64() read_64_(0)
#ifdef ckd_have_int128
#  define read_128() (((tmp_128)read_64_(0) << 64) | (tmp_128)read_64_(1))
#endif

#define STRINGIFY_BUFFER 50

static char c1[STRINGIFY_BUFFER];
static char c2[STRINGIFY_BUFFER];
static char c3[STRINGIFY_BUFFER];
static char c4[STRINGIFY_BUFFER];

static void report_mismatch(char o1, char o2, int i1, int i2, int i3, int i4)
{
  char const* msg =
      "Mismatch @ 0x%lX\n  Actual: (%c) %s\n  Expected: (%c) %s\n  Types: T = "
      "%s, U = %s, V = %s\n  Operation: %s(%s, %s)\n  Vector indices: i = %d, "
      "j = %d\n";
#define args \
  offset, '0' + o1, c1 + i1, '0' + o2, c2 + i2, t_type, u_type, v_type, op, \
      c3 + i3, c4 + i4, i, j
  assert(!(fprintf(stderr, msg, args) < 0));
}

#define SIGNED_int 1
#define SIGNED_uint 0

#define XCAT(x, y) x##y
#define CAT(x, y) XCAT(x, y)
#define IF(c) CAT(IF_, c)
#define IF_0(t, ...) __VA_ARGS__
#define IF_1(t, ...) t
#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)

#define DECLARE_MISMATCH(S, N) \
  static int stringify_##S##N##_t(void const* x_, char* c) \
  { \
    S##N##_t x = *(S##N##_t const*)x_; \
    int p = STRINGIFY_BUFFER; \
    WHEN(SIGNED_##S)(int const s = x < 0 ? -1 : 1); \
    c[--p] = 0; \
    do { \
      S##N##_t tmp = x % 10; \
      c[--p] = '0' + (char)(WHEN(SIGNED_##S)(s == -1 ? -tmp :) tmp); \
      x /= 10; \
    } while (x != 0); \
    WHEN(SIGNED_##S)(if (s != 1) c[--p] = '-'); \
    return p; \
  } \
  static char mismatch_##S##N##_t(char o1, S##N##_t z1) \
  { \
    char o2 = (ref & 0x40) != 0; \
    S##N##_t z2 = (S##N##_t)read_##N(); \
    if (o1 == o2 && z1 == z2) { \
      return 0; \
    } \
    report_mismatch(o1, \
                    o2, \
                    stringify_##S##N##_t(&z1, c1), \
                    stringify_##S##N##_t(&z2, c2), \
                    u_stringify(u_ptr, c3), \
                    v_stringify(v_ptr, c4)); \
    return 1; \
  }

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4296; disable : 4146)
#endif
DECLARE_MISMATCH(int, 8)
DECLARE_MISMATCH(uint, 8)
DECLARE_MISMATCH(int, 16)
DECLARE_MISMATCH(uint, 16)
DECLARE_MISMATCH(int, 32)
DECLARE_MISMATCH(uint, 32)
DECLARE_MISMATCH(int, 64)
DECLARE_MISMATCH(uint, 64)
#ifdef ckd_have_int128
DECLARE_MISMATCH(int, 128)
DECLARE_MISMATCH(uint, 128)
#endif
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#ifdef ckd_have_int128
static void read_next(void)
{
  offset = ftell(reference);
  assert(fread(&ref, 1, 1, reference) == 1);
  size = ref & 0x3F;
  assert(fread(buffer, 1, size, reference) == size);
}
#else
static void read_next(void)
{
  offset = ftell(reference);
  while (1) {
    assert(fread(&ref, 1, 1, reference) == 1);
    size = ref & 0x3F;
    if ((ref & 0x80) == 0) {
      assert(fread(buffer, 1, size, reference) == size);
      return;
    }
    assert(!fseek(reference, (long)size, SEEK_CUR));
    offset += 1 + (long)size;
  }
}
#endif

char test_odr(int a, int b);

int main(int argc, char* argv[])
{
  char o = 0;

  (void)argc;
  (void)argv;

  o = test_odr(1, -1);
  if (!o) {
    return 1;
  }

  reference = fopen("test.bin", "rb");
  assert(reference);

#define check_next(T, f) \
  do { \
    op = #f; \
    read_next(); \
    o = (char)(int)f(&z, x, y); \
    if (mismatch_##T(o, z)) { \
      return 1; \
    } \
  } while (0)

#define M(T, U, V) \
  t_type = #T; \
  u_type = #U; \
  v_type = #V; \
  for (i = 0; i != (int)(sizeof(k##U) / sizeof(k##U[0])); ++i) { \
    U x = k##U[i]; \
    u_ptr = &x; \
    u_stringify = stringify_##U; \
    for (j = 0; j != (int)(sizeof(k##V) / sizeof(k##V[0])); ++j) { \
      T z; \
      V y = k##V[j]; \
      v_ptr = &y; \
      v_stringify = stringify_##V; \
      check_next(T, ckd_add); \
      check_next(T, ckd_sub); \
      check_next(T, ckd_mul); \
    } \
  }

#ifdef ckd_have_int128
#  define MM(T, U) \
    M(T, U, uint8_t) \
    M(T, U, uint16_t) \
    M(T, U, uint32_t) \
    M(T, U, uint64_t) \
    M(T, U, uint128_t) \
    M(T, U, int8_t) \
    M(T, U, int16_t) \
    M(T, U, int32_t) \
    M(T, U, int64_t) \
    M(T, U, int128_t)
#  define MMM(T) \
    MM(T, uint8_t) \
    MM(T, uint16_t) \
    MM(T, uint32_t) \
    MM(T, uint64_t) \
    MM(T, uint128_t) \
    MM(T, int8_t) \
    MM(T, int16_t) \
    MM(T, int32_t) \
    MM(T, int64_t) \
    MM(T, int128_t)
  MMM(uint8_t)
  MMM(uint16_t)
  MMM(uint32_t)
  MMM(uint64_t)
  MMM(uint128_t)
  MMM(int8_t)
  MMM(int16_t)
  MMM(int32_t)
  MMM(int64_t)
  MMM(int128_t)

#else
#  define MM(T, U) \
    M(T, U, uint8_t) \
    M(T, U, uint16_t) \
    M(T, U, uint32_t) \
    M(T, U, uint64_t) \
    M(T, U, int8_t) \
    M(T, U, int16_t) \
    M(T, U, int32_t) \
    M(T, U, int64_t)
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
  while (1) {
    if (fread(&ref, 1, 1, reference) != 1) {
      break;
    }
    assert(ref & 0x80);
    assert(!fseek(reference, ref & 0x3F, SEEK_CUR));
  }
#endif

  if (fgetc(reference) != EOF || !feof(reference)) {
    long current = ftell(reference);
    long end = 0;
    assert(!fseek(reference, 0, SEEK_END));
    end = ftell(reference);
#define msg "Reference was not read to completion. %ld bytes left.\n"
    assert(!(fprintf(stderr, msg, end - current) < 0));
#undef msg
    return 1;
  }

  assert(fclose(reference) == 0);
  return 0;
}
