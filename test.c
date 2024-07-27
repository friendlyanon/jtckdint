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

int main(int argc, char const* argv[])
{
  (void)argc;
  (void)argv;

  reference = fopen("test.bin", "rb");
  assert(reference);

#ifdef ckd_have_int128
#  define read() \
    do { \
      assert(fread(&ref, 1, 1, reference) == 1); \
      size = ref & 0x3F; \
      assert(fread(&buffer, 1, size, reference) == size); \
    } while (0)
#else
#  define read() \
    do { \
      assert(fread(&ref, 1, 1, reference) == 1); \
      size = ref & 0x3F; \
      if ((ref & 0x80) == 0) { \
        assert(fread(&buffer, 1, size, reference) == size); \
        break; \
      } \
      assert(!fseek(reference, (long)size, SEEK_CUR)); \
      offset += 1 + (long)size; \
    } while (1)
#endif

#define stringify(N) \
  do { \
    int s = z##N < 0 ? -1 : 1; \
    i##N = sizeof(c##N); \
    c##N[--i##N] = 0; \
    do { \
      c##N[--i##N] = '0' + (char)(s * (z##N % 10)); \
      z##N /= 10; \
    } while (z##N != 0); \
    if (s == -1) { \
      c##N[--i##N] = '-'; \
    } \
  } while (0)

#define check_next(expr) \
  do { \
    int o1, o2; \
    size_t size = 0, index = 0; \
    unsigned char buffer[16]; \
    unsigned char ref = 0; \
    long offset = ftell(reference); \
    read(); \
    z2 = buffer[index++]; \
    while (index != size) { \
      z2 = (z2 << 8) | buffer[index++]; \
    } \
    o1 = !!(expr); \
    o2 = (ref & 0x40) != 0; \
    if (o1 != o2 || z1 != z2) { \
      char c1[50], c2[50]; \
      int i1, i2; \
      stringify(1); \
      stringify(2); \
      assert(!(fprintf(stderr, \
                       "Mismatch @ 0x%lX: (%c) %s != (%c) %s\n", \
                       offset, \
                       '0' + o1, \
                       c1 + i1, \
                       '0' + o2, \
                       c2 + i2) \
               < 0)); \
      return 1; \
    } \
  } while (0)

#define M(T, U, V) \
  for (int i = 0; i != (int)(sizeof(k##U) / sizeof(k##U[0])); ++i) { \
    U x = k##U[i]; \
    for (int j = 0; j != (int)(sizeof(k##V) / sizeof(k##V[0])); ++j) { \
      T z1, z2; \
      V y = k##V[j]; \
      check_next(ckd_add(&z1, x, y)); \
      check_next(ckd_sub(&z1, x, y)); \
      check_next(ckd_mul(&z1, x, y)); \
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
    unsigned char ref = 0;
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
