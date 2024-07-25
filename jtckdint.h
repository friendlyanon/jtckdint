/*
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

/**
 * @fileoverview C23 Checked Arithmetic
 *
 * This header defines three type generic functions:
 *
 *   - `bool ckd_add(res, a, b)`
 *   - `bool ckd_sub(res, a, b)`
 *   - `bool ckd_mul(res, a, b)`
 *
 * Which allow integer arithmetic errors to be detected. There are many
 * kinds of integer errors, e.g. overflow, truncation, etc. These funcs
 * catch them all. Here's an example of how it works:
 *
 *     uint32_t c;
 *     int32_t a = 0x7fffffff;
 *     int32_t b = 2;
 *     assert(!ckd_add(&c, a, b));
 *     assert(c == 0x80000001u);
 *
 * Experienced C / C++ users should find this example counter-intuitive
 * because the expression `0x7fffffff + 2` not only overflows it's also
 * undefined behavior. However here we see it's specified, and does not
 * result in an error. That's because C23 checked arithmetic is not the
 * arithmetic you're used to. The new standard changes the mathematics.
 *
 * C23 checked arithmetic is defined as performing the arithmetic using
 * infinite precision and then checking if the resulting value will fit
 * in the output type. Our example above did not result in an error due
 * to `0x80000001` being a legal value for `uint32_t`.
 *
 * This implementation will use the GNU compiler builtins, when they're
 * available, only if you don't use build flags like `-std=c11` because
 * they define `__STRICT_ANSI__` and GCC extensions aren't really ANSI.
 * Instead, you'll get a pretty good pure C11 and C++11 implementation.
 *
 * @see https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf
 * @version 0.1 (2023-07-22)
 */

#ifndef JTCKDINT_H_
#define JTCKDINT_H_

#ifdef __has_include
#  define ckd_has_include(x) __has_include(x)
#else
#  define ckd_has_include(x) 0
#endif

#if ckd_has_include(<stdckdint.h>)
#  include <stdckdint.h>
#else

#  if (defined(__llvm__) \
       || (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 406)) \
      && !defined(__STRICT_ANSI__)
#    define ckd_have_int128
#    define ckd_intmax __int128
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define ckd_intmax long long
#  else
#    define ckd_intmax long
#  endif

typedef signed ckd_intmax ckd_intmax_t;
typedef unsigned ckd_intmax ckd_uintmax_t;

#  ifdef __has_builtin
#    define ckd_has_builtin(x) __has_builtin(x)
#  else
#    define ckd_has_builtin(x) 0
#  endif

#  if !defined(__STRICT_ANSI__) \
      && (defined(__GNUC__) && __GNUC__ >= 5 && !defined(__ICC) \
          || ckd_has_builtin(__builtin_add_overflow) \
              && ckd_has_builtin(__builtin_sub_overflow) \
              && ckd_has_builtin(__builtin_mul_overflow))
#    define ckd_add(res, x, y) __builtin_add_overflow((x), (y), (res))
#    define ckd_sub(res, x, y) __builtin_sub_overflow((x), (y), (res))
#    define ckd_mul(res, x, y) __builtin_mul_overflow((x), (y), (res))

#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#    define ckd_add(res, a, b) ckd_expr(add, (res), (a), (b))
#    define ckd_sub(res, a, b) ckd_expr(sub, (res), (a), (b))
#    define ckd_mul(res, a, b) ckd_expr(mul, (res), (a), (b))

#    if defined(__GNUC__) || defined(__llvm__)
#      define ckd_inline \
        extern __inline \
            __attribute__((__gnu_inline__, __always_inline__, __artificial__))
#    else
#      define ckd_inline static inline
#    endif

#    ifdef ckd_have_int128
/* clang-format off */
#      define ckd_generic_int128(x, y) \
        , signed __int128: x \
        , unsigned __int128: y
/* clang-format on */
#    else
#      define ckd_generic_int128(x, y)
#    endif

#    define ckd_sign(T) ((T)1 << (sizeof(T) * 8 - 1))

#    define ckd_is_signed(x) \
      _Generic(x, \
          signed char: 1, \
          unsigned char: 0, \
          signed short: 1, \
          unsigned short: 0, \
          signed int: 1, \
          unsigned int: 0, \
          signed long: 1, \
          unsigned long: 0, \
          signed long long: 1, \
          unsigned long long: 0 ckd_generic_int128(1, 0))

#    define ckd_expr(op, res, a, b) \
      (_Generic(*res, \
          signed char: ckd_##op##_schar, \
          unsigned char: ckd_##op##_uchar, \
          signed short: ckd_##op##_sshort, \
          unsigned short: ckd_##op##_ushort, \
          signed int: ckd_##op##_sint, \
          unsigned int: ckd_##op##_uint, \
          signed long: ckd_##op##_slong, \
          unsigned long: ckd_##op##_ulong, \
          signed long long: ckd_##op##_slonger, \
          unsigned long long: ckd_##op##_ulonger ckd_generic_int128( \
              ckd_##op##_sint128, ckd_##op##_uint128))( \
          res, a, b, ckd_is_signed(a), ckd_is_signed(b)))

#    define ckd_declare_add(S, T) \
      ckd_inline char S(void* res, \
                        ckd_uintmax_t x, \
                        ckd_uintmax_t y, \
                        char a_signed, \
                        char b_signed) \
      { \
        ckd_uintmax_t z = x + y; \
        *(T*)res = z; \
        char truncated = 0; \
        if (sizeof(T) < sizeof(ckd_intmax_t)) { \
          truncated = z != (ckd_uintmax_t)(T)z; \
        } \
        switch (ckd_is_signed((T)0) << 2 | a_signed << 1 | b_signed) { \
          case 0: /* u = u + u */ \
            return truncated | (z < x); \
          case 1: /* u = u + s */ \
            y ^= ckd_sign(ckd_uintmax_t); \
            return truncated | ((ckd_intmax_t)((z ^ x) & (z ^ y)) < 0); \
          case 2: /* u = s + u */ \
            x ^= ckd_sign(ckd_uintmax_t); \
            return truncated | ((ckd_intmax_t)((z ^ x) & (z ^ y)) < 0); \
          case 3: /* u = s + s */ \
            return truncated \
                | ((ckd_intmax_t)(((z | x) & y) | ((z & x) & ~y)) < 0); \
          case 4: /* s = u + u */ \
            return truncated | (z < x) | ((ckd_intmax_t)z < 0); \
          case 5: /* s = u + s */ \
            y ^= ckd_sign(ckd_uintmax_t); \
            return truncated | (x + y < y); \
          case 6: /* s = s + u */ \
            x ^= ckd_sign(ckd_uintmax_t); \
            return truncated | (x + y < x); \
          case 7: /* s = s + s */ \
            return truncated | ((ckd_intmax_t)((z ^ x) & (z ^ y)) < 0); \
          default: \
            for (;;) \
              (void)0; \
        } \
      }

/* clang-format off */
ckd_declare_add(ckd_add_schar, signed char)
ckd_declare_add(ckd_add_uchar, unsigned char)
ckd_declare_add(ckd_add_sshort, signed short)
ckd_declare_add(ckd_add_ushort, unsigned short)
ckd_declare_add(ckd_add_sint, signed int)
ckd_declare_add(ckd_add_uint, unsigned int)
ckd_declare_add(ckd_add_slong, signed long)
ckd_declare_add(ckd_add_ulong, unsigned long)
ckd_declare_add(ckd_add_slonger, signed long long)
ckd_declare_add(ckd_add_ulonger, unsigned long long)
#    ifdef ckd_have_int128
ckd_declare_add(ckd_add_sint128, signed __int128)
ckd_declare_add(ckd_add_uint128, unsigned __int128)
#    endif
/* clang-format on */

#    define ckd_declare_sub(S, T) \
      ckd_inline char S(void* res, \
                        ckd_uintmax_t x, \
                        ckd_uintmax_t y, \
                        char a_signed, \
                        char b_signed) \
      { \
        ckd_uintmax_t z = x - y; \
        *(T*)res = z; \
        char truncated = 0; \
        if (sizeof(T) < sizeof(ckd_intmax_t)) { \
          truncated = z != (ckd_uintmax_t)(T)z; \
        } \
        switch (ckd_is_signed((T)0) << 2 | a_signed << 1 | b_signed) { \
          case 0: /* u = u - u */ \
            return truncated | (x < y); \
          case 1: /* u = u - s */ \
            y ^= ckd_sign(ckd_uintmax_t); \
            return truncated | ((ckd_intmax_t)((x ^ y) & (z ^ x)) < 0); \
          case 2: /* u = s - u */ \
            return truncated | (y > x) | ((ckd_intmax_t)x < 0); \
          case 3: /* u = s - s */ \
            return truncated \
                | ((ckd_intmax_t)(((z & x) & y) | ((z | x) & ~y)) < 0); \
          case 4: /* s = u - u */ \
            return truncated | ((x < y) ^ ((ckd_intmax_t)z < 0)); \
          case 5: /* s = u - s */ \
            y ^= ckd_sign(ckd_uintmax_t); \
            return truncated | (x >= y); \
          case 6: /* s = s - u */ \
            x ^= ckd_sign(ckd_uintmax_t); \
            return truncated | (x < y); \
          case 7: /* s = s - s */ \
            return truncated | ((ckd_intmax_t)((x ^ y) & (z ^ x)) < 0); \
          default: \
            for (;;) \
              (void)0; \
        } \
      }

    /* clang-format off */
ckd_declare_sub(ckd_sub_schar, signed char)
ckd_declare_sub(ckd_sub_uchar, unsigned char)
ckd_declare_sub(ckd_sub_sshort, signed short)
ckd_declare_sub(ckd_sub_ushort, unsigned short)
ckd_declare_sub(ckd_sub_sint, signed int)
ckd_declare_sub(ckd_sub_uint, unsigned int)
ckd_declare_sub(ckd_sub_slong, signed long)
ckd_declare_sub(ckd_sub_ulong, unsigned long)
ckd_declare_sub(ckd_sub_slonger, signed long long)
ckd_declare_sub(ckd_sub_ulonger, unsigned long long)
#    ifdef ckd_have_int128
ckd_declare_sub(ckd_sub_sint128, signed __int128)
ckd_declare_sub(ckd_sub_uint128, unsigned __int128)
#    endif
/* clang-format on */

#    define ckd_declare_mul(S, T) \
      ckd_inline char S(void* res, \
                        ckd_uintmax_t x, \
                        ckd_uintmax_t y, \
                        char a_signed, \
                        char b_signed) \
      { \
        switch (ckd_is_signed((T)0) << 2 | a_signed << 1 | b_signed) { \
          case 0: { /* u = u * u */ \
            ckd_uintmax_t z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = z; \
            return o \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res); \
          } \
          case 1: { /* u = u * s */ \
            ckd_uintmax_t z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = z; \
            return ( \
                o | (((ckd_intmax_t)y < 0) & !!x) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res)); \
          } \
          case 2: { /* u = s * u */ \
            ckd_uintmax_t z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = z; \
            return ( \
                o | (((ckd_intmax_t)x < 0) & !!y) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res)); \
          } \
          case 3: { /* u = s * s */ \
            int o = 0; \
            if ((ckd_intmax_t)(x & y) < 0) { \
              x = -x; \
              y = -y; \
            } else if ((ckd_intmax_t)(x ^ y) < 0) { \
              o = x && y; \
            } \
            ckd_uintmax_t z = x * y; \
            o |= x && z / x != y; \
            *(T*)res = z; \
            return o \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res); \
          } \
          case 4: { /* s = u * u */ \
            ckd_uintmax_t z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = z; \
            return ( \
                o | ((ckd_intmax_t)(z) < 0) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res)); \
          } \
          case 5: { /* s = u * s */ \
            ckd_uintmax_t t = -y; \
            t = (ckd_intmax_t)(t) < 0 ? y : t; \
            ckd_uintmax_t p = t * x; \
            int o = t && p / t != x; \
            int n = (ckd_intmax_t)y < 0; \
            ckd_uintmax_t z = n ? -p : p; \
            *(T*)res = z; \
            ckd_uintmax_t m = ckd_sign(ckd_uintmax_t) - 1; \
            return ( \
                o | (p > m + n) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res)); \
          } \
          case 6: { /* s = s * u */ \
            ckd_uintmax_t t = -x; \
            t = (ckd_intmax_t)(t) < 0 ? x : t; \
            ckd_uintmax_t p = t * y; \
            int o = t && p / t != y; \
            int n = (ckd_intmax_t)x < 0; \
            ckd_uintmax_t z = n ? -p : p; \
            *(T*)res = z; \
            ckd_uintmax_t m = ckd_sign(ckd_uintmax_t) - 1; \
            return ( \
                o | (p > m + n) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res)); \
          } \
          case 7: { /* s = s * s */ \
            ckd_uintmax_t z = x * y; \
            *(T*)res = z; \
            return ( \
                ((((ckd_intmax_t)y < 0) && (x == ckd_sign(ckd_uintmax_t))) \
                 || (y \
                     && (((ckd_intmax_t)z / (ckd_intmax_t)y) \
                         != (ckd_intmax_t)x))) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax_t) * (T*)res)); \
          } \
          default: \
            for (;;) \
              (void)0; \
        } \
      }

    /* clang-format off */
ckd_declare_mul(ckd_mul_schar, signed char)
ckd_declare_mul(ckd_mul_uchar, unsigned char)
ckd_declare_mul(ckd_mul_sshort, signed short)
ckd_declare_mul(ckd_mul_ushort, unsigned short)
ckd_declare_mul(ckd_mul_sint, signed int)
ckd_declare_mul(ckd_mul_uint, unsigned int)
ckd_declare_mul(ckd_mul_slong, signed long)
ckd_declare_mul(ckd_mul_ulong, unsigned long)
ckd_declare_mul(ckd_mul_slonger, signed long long)
ckd_declare_mul(ckd_mul_ulonger, unsigned long long)
#    ifdef ckd_have_int128
ckd_declare_mul(ckd_mul_sint128, signed __int128)
ckd_declare_mul(ckd_mul_uint128, unsigned __int128)
#    endif
/* clang-format on */

#  else
#    pragma message "checked integer arithmetic unsupported in this environment"

#    define ckd_add(res, x, y) (*(res) = (x) + (y), 0)
#    define ckd_sub(res, x, y) (*(res) = (x) - (y), 0)
#    define ckd_mul(res, x, y) (*(res) = (x) * (y), 0)

#  endif /* GNU */
#endif /* stdckdint.h */
#endif /* JTCKDINT_H_ */
