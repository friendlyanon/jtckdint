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
#include <string.h>

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

static void byte_swap(void)
{
  short test = 1;
  unsigned char byte;
  memcpy(&byte, &test, 1);
  if (byte == 0) {
    return;
  }

  {
    int begin = 0;
    int end = (int)size;
    while (--end > begin) {
      unsigned char tmp = buffer[begin];
      buffer[begin++] = buffer[end];
      buffer[end] = tmp;
    }
  }
}

#define STRINGIFY_BUFFER 50

#define DECLARE_MISMATCH(T) \
  static int stringify_##T(T x, char* c) \
  { \
    int const s = x < 0 ? -1 : 1; \
    int i = STRINGIFY_BUFFER; \
    c[--i] = 0; \
    do { \
      T tmp = x % 10; \
      c[--i] = '0' + (char)(s == -1 ? -tmp : tmp); \
      x /= 10; \
    } while (x != 0); \
    if (s == -1) { \
      c[--i] = '-'; \
    } \
    return i; \
  } \
  static int mismatch_##T(int o1, T z1) \
  { \
    char c1[STRINGIFY_BUFFER]; \
    char c2[STRINGIFY_BUFFER]; \
    T z2; \
    byte_swap(); \
    memcpy(&z2, buffer, size); \
    if (o1 == ((ref & 0x40) != 0) && z1 == z2) { \
      return 0; \
    } \
    assert(!(fprintf(stderr, \
                     "Mismatch @ 0x%lX: (%c) %s != (%c) %s\n", \
                     offset, \
                     '0' + o1, \
                     c1 + stringify_##T(z1, c1), \
                     '0' + ((ref & 0x40) != 0), \
                     c2 + stringify_##T(z2, c2)) \
             < 0)); \
    return 1; \
  }

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4296; disable : 4146)
#endif
DECLARE_MISMATCH(int8_t)
DECLARE_MISMATCH(uint8_t)
DECLARE_MISMATCH(int16_t)
DECLARE_MISMATCH(uint16_t)
DECLARE_MISMATCH(int32_t)
DECLARE_MISMATCH(uint32_t)
DECLARE_MISMATCH(int64_t)
DECLARE_MISMATCH(uint64_t)
#ifdef ckd_have_int128
DECLARE_MISMATCH(int128_t)
DECLARE_MISMATCH(uint128_t)
#endif
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#ifdef ckd_have_int128
static void read_ref(void)
{
  offset = ftell(reference);
  assert(fread(&ref, 1, 1, reference) == 1);
  size = ref & 0x3F;
  assert(fread(buffer, 1, size, reference) == size);
}
#else
static void read_ref(void)
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

int main(int argc, char* argv[])
{
  int o;

  (void)argc;
  (void)argv;

  reference = fopen("test.bin", "rb");
  assert(reference);

#define check_next(T, op) \
  do { \
    read_ref(); \
    o = op(&z, x, y); \
    if (mismatch_##T(o, z)) { \
      return 1; \
    } \
  } while (0)

#define M(T, U, V) \
  for (int i = 0; i != (int)(sizeof(k##U) / sizeof(k##U[0])); ++i) { \
    U x = k##U[i]; \
    for (int j = 0; j != (int)(sizeof(k##V) / sizeof(k##V[0])); ++j) { \
      T z; \
      V y = k##V[j]; \
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
    assert(!(fprintf(stderr,
                     "Reference was not read to completion. "
                     "%ld bytes left.\n",
                     end - current)
             < 0));
    return 1;
  }

  assert(fclose(reference) == 0);
  return 0;
}
