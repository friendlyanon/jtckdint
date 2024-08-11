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

#ifdef __has_feature
#  define ckd_has_feature(x) __has_feature(x)
#else
#  define ckd_has_feature(x) 0
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L \
    && ckd_has_include(<stdckdint.h>)
#  include <stdckdint.h>
#else

#  if (defined(__llvm__) \
       || (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 406)) \
      && !defined(__STRICT_ANSI__)
#    define ckd_have_int128
#    define ckd_longest __int128
#  elif (defined(__cplusplus) && __cplusplus >= 201103L) \
      || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#    define ckd_longest long long
#  else
#    define ckd_longest long
#  endif

typedef signed ckd_longest ckd_intmax;
typedef unsigned ckd_longest ckd_uintmax;

#  ifdef __has_builtin
#    define ckd_has_builtin(x) __has_builtin(x)
#  else
#    define ckd_has_builtin(x) 0
#  endif

#  if defined(__GNUC__) || defined(__llvm__)
#    define ckd_unreachable(x) __builtin_unreachable()
#  elif defined(_MSC_VER)
#    define ckd_unreachable(x) __assume(0)
#  else
#    define ckd_unreachable(x) return (x)
#  endif

#  if !defined(__STRICT_ANSI__) \
      && (defined(__GNUC__) && __GNUC__ >= 5 && !defined(__ICC) \
          || ckd_has_builtin(__builtin_add_overflow) \
              && ckd_has_builtin(__builtin_sub_overflow) \
              && ckd_has_builtin(__builtin_mul_overflow))
#    define ckd_add(res, x, y) __builtin_add_overflow((x), (y), (res))
#    define ckd_sub(res, x, y) __builtin_sub_overflow((x), (y), (res))
#    define ckd_mul(res, x, y) __builtin_mul_overflow((x), (y), (res))

#  elif defined(__cplusplus) \
      && (__cplusplus >= 201103L \
          || defined(_MSC_VER) && __cplusplus >= 199711L \
              && ckd_has_include(<type_traits>) && ckd_has_include(<limits>))
#    include <limits>
#    include <type_traits>

#    if defined(__GNUC__) || defined(__llvm__)
#      define ckd_inline \
        inline __attribute__((__always_inline__, __artificial__))
#    elif defined(_MSC_VER)
#      define ckd_inline __forceinline
#    else
#      define ckd_inline inline
#    endif

#    if (defined(_MSC_VER) && defined(_MSVC_LANG) && _MSC_VER >= 1915 \
         && _MSVC_LANG >= 201402L) \
        || (defined(__llvm__) && ckd_has_feature(__cxx_generic_lambdas__) \
            && ckd_has_feature(__cxx_relaxed_constexpr__)) \
        || (defined(__cpp_constexpr) && (__cpp_constexpr >= 201304L))
#      define ckd_constexpr constexpr
#    else
#      define ckd_constexpr
#    endif

template<typename T, typename U, typename V>
ckd_constexpr ckd_inline bool ckd_add(T* res, U a, V b)
{
  static_assert(std::is_integral<T>::value && std::is_integral<U>::value
                    && std::is_integral<V>::value,
                "non-integral types not allowed");
  static_assert(!std::is_same<T, bool>::value && !std::is_same<U, bool>::value
                    && !std::is_same<V, bool>::value,
                "checked booleans not supported");
  static_assert(!std::is_same<T, char>::value && !std::is_same<U, char>::value
                    && !std::is_same<V, char>::value,
                "unqualified char type is ambiguous");
  auto x = static_cast<ckd_uintmax>(a);
  auto y = static_cast<ckd_uintmax>(b);
  auto z = x + y;
  *res = static_cast<T>(z);
  if (sizeof(z) > sizeof(U) && sizeof(z) > sizeof(V)) {
    if (sizeof(z) > sizeof(T) || std::is_signed<T>::value) {
      return static_cast<ckd_intmax>(z) != static_cast<T>(z);
    } else if (!std::is_same<T, ckd_uintmax>::value) {
      return (z != static_cast<T>(z)
              || ((std::is_signed<U>::value || std::is_signed<V>::value)
                  && static_cast<ckd_intmax>(z) < 0));
    }
  }
  bool truncated = false;
  if (sizeof(T) < sizeof(ckd_intmax)) {
    truncated = z != static_cast<ckd_uintmax>(static_cast<T>(z));
  }
  switch (std::is_signed<T>::value << 2 |  //
          std::is_signed<U>::value << 1 |  //
          std::is_signed<V>::value)
  {
    case 0:  // u = u + u
      return truncated | (z < x);
    case 1:  // u = u + s
      y ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (static_cast<ckd_intmax>((z ^ x) & (z ^ y)) < 0);
    case 2:  // u = s + u
      x ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (static_cast<ckd_intmax>((z ^ x) & (z ^ y)) < 0);
    case 3:  // u = s + s
      return truncated
          | (static_cast<ckd_intmax>(((z | x) & y) | ((z & x) & ~y)) < 0);
    case 4:  // s = u + u
      return truncated | (z < x) | (static_cast<ckd_intmax>(z) < 0);
    case 5:  // s = u + s
      y ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (x + y < y);
    case 6:  // s = s + u
      x ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (x + y < x);
    case 7:  // s = s + s
      return truncated | (static_cast<ckd_intmax>((z ^ x) & (z ^ y)) < 0);
    default:
      ckd_unreachable(false);
  }
}

template<typename T, typename U, typename V>
ckd_constexpr ckd_inline bool ckd_sub(T* res, U a, V b)
{
  static_assert(std::is_integral<T>::value && std::is_integral<U>::value
                    && std::is_integral<V>::value,
                "non-integral types not allowed");
  static_assert(!std::is_same<T, bool>::value && !std::is_same<U, bool>::value
                    && !std::is_same<V, bool>::value,
                "checked booleans not supported");
  static_assert(!std::is_same<T, char>::value && !std::is_same<U, char>::value
                    && !std::is_same<V, char>::value,
                "unqualified char type is ambiguous");
  auto x = static_cast<ckd_uintmax>(a);
  auto y = static_cast<ckd_uintmax>(b);
  auto z = x - y;
  *res = static_cast<T>(z);
  if (sizeof(z) > sizeof(U) && sizeof(z) > sizeof(V)) {
    if (sizeof(z) > sizeof(T) || std::is_signed<T>::value) {
      return static_cast<ckd_intmax>(z) != static_cast<T>(z);
    } else if (!std::is_same<T, ckd_uintmax>::value) {
      return (z != static_cast<T>(z)
              || ((std::is_signed<U>::value || std::is_signed<V>::value)
                  && static_cast<ckd_intmax>(z) < 0));
    }
  }
  bool truncated = false;
  if (sizeof(T) < sizeof(ckd_intmax)) {
    truncated = z != static_cast<ckd_uintmax>(static_cast<T>(z));
  }
  switch (std::is_signed<T>::value << 2 |  //
          std::is_signed<U>::value << 1 |  //
          std::is_signed<V>::value)
  {
    case 0:  // u = u - u
      return truncated | (x < y);
    case 1:  // u = u - s
      y ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (static_cast<ckd_intmax>((x ^ y) & (z ^ x)) < 0);
    case 2:  // u = s - u
      return truncated | (y > x) | (static_cast<ckd_intmax>(x) < 0);
    case 3:  // u = s - s
      return truncated
          | (static_cast<ckd_intmax>(((z & x) & y) | ((z | x) & ~y)) < 0);
    case 4:  // s = u - u
      return truncated | ((x < y) ^ (static_cast<ckd_intmax>(z) < 0));
    case 5:  // s = u - s
      y ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (x >= y);
    case 6:  // s = s - u
      x ^= static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::min)());
      return truncated | (x < y);
    case 7:  // s = s - s
      return truncated | (static_cast<ckd_intmax>((x ^ y) & (z ^ x)) < 0);
    default:
      ckd_unreachable(false);
  }
}

template<typename T, typename U, typename V>
ckd_constexpr ckd_inline bool ckd_mul(T* res, U a, V b)
{
  static_assert(std::is_integral<T>::value && std::is_integral<U>::value
                    && std::is_integral<V>::value,
                "non-integral types not allowed");
  static_assert(!std::is_same<T, bool>::value && !std::is_same<U, bool>::value
                    && !std::is_same<V, bool>::value,
                "checked booleans not supported");
  static_assert(!std::is_same<T, char>::value && !std::is_same<U, char>::value
                    && !std::is_same<V, char>::value,
                "unqualified char type is ambiguous");
  auto x = static_cast<ckd_uintmax>(a);
  auto y = static_cast<ckd_uintmax>(b);
  if ((sizeof(U) * 8 - std::is_signed<U>::value)
          + (sizeof(V) * 8 - std::is_signed<V>::value)
      <= (sizeof(T) * 8 - std::is_signed<T>::value))
  {
    if (sizeof(ckd_uintmax) > sizeof(T) || std::is_signed<T>::value) {
      auto z = static_cast<ckd_intmax>(x * y);
      return z != (*res = static_cast<T>(z));
    } else if (!std::is_same<T, ckd_uintmax>::value) {
      auto z = x * y;
      *res = static_cast<T>(z);
      return (z != static_cast<T>(z)
              || ((std::is_signed<U>::value || std::is_signed<V>::value)
                  && static_cast<ckd_intmax>(z) < 0));
    }
  }
  switch (std::is_signed<T>::value << 2 |  //
          std::is_signed<U>::value << 1 |  //
          std::is_signed<V>::value)
  {
    case 0: {  // u = u * u
      auto z = x * y;
      bool o = x && z / x != y;
      *res = static_cast<T>(z);
      return o | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res));
    }
    case 1: {  // u = u * s
      auto z = x * y;
      bool o = x && z / x != y;
      *res = static_cast<T>(z);
      return (o | ((static_cast<ckd_intmax>(y) < 0) & !!x)
              | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res)));
    }
    case 2: {  // u = s * u
      auto z = x * y;
      bool o = x && z / x != y;
      *res = static_cast<T>(z);
      return (o | ((static_cast<ckd_intmax>(x) < 0) & !!y)
              | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res)));
    }
    case 3: {  // u = s * s
      bool o = false;
      if (static_cast<ckd_intmax>(x & y) < 0) {
        x = -x;
        y = -y;
      } else if (static_cast<ckd_intmax>(x ^ y) < 0) {
        o = x && y;
      }
      auto z = x * y;
      o |= x && z / x != y;
      *res = static_cast<T>(z);
      return o | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res));
    }
    case 4: {  // s = u * u
      auto z = x * y;
      bool o = x && z / x != y;
      *res = static_cast<T>(z);
      return (o | (static_cast<ckd_intmax>(z) < 0)
              | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res)));
    }
    case 5: {  // s = u * s
      auto t = -y;
      t = static_cast<ckd_intmax>(t) < 0 ? y : t;
      auto p = t * x;
      bool o = t && p / t != x;
      bool n = static_cast<ckd_intmax>(y) < 0;
      auto z = n ? -p : p;
      *res = static_cast<T>(z);
      auto m =
          static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::max)());
      return (o | (p > m + n)
              | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res)));
    }
    case 6: {  // s = s * u
      auto t = -x;
      t = static_cast<ckd_intmax>(t) < 0 ? x : t;
      auto p = t * y;
      bool o = t && p / t != y;
      bool n = static_cast<ckd_intmax>(x) < 0;
      auto z = n ? -p : p;
      *res = static_cast<T>(z);
      auto m =
          static_cast<ckd_uintmax>((std::numeric_limits<ckd_intmax>::max)());
      return (o | (p > m + n)
              | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res)));
    }
    case 7: {  // s = s * s
      auto z = x * y;
      *res = static_cast<T>(z);
      return ((((static_cast<ckd_intmax>(y) < 0)
                && (static_cast<ckd_intmax>(x)
                    == (std::numeric_limits<ckd_intmax>::min)()))
               || (y
                   && ((static_cast<ckd_intmax>(z) / static_cast<ckd_intmax>(y))
                       != static_cast<ckd_intmax>(x))))
              | (sizeof(T) < sizeof(z) && z != static_cast<ckd_uintmax>(*res)));
    }
    default:
      ckd_unreachable(false);
  }
}

#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

#    define ckd_add(res, a, b) ckd_expr(add, (res), (a), (b))
#    define ckd_sub(res, a, b) ckd_expr(sub, (res), (a), (b))
#    define ckd_mul(res, a, b) ckd_expr(mul, (res), (a), (b))

#    if defined(__GNUC__) || defined(__llvm__)
#      define ckd_inline \
        extern __inline \
            __attribute__((__gnu_inline__, __always_inline__, __artificial__))
#    elif defined(_MSC_VER)
#      define ckd_inline static __forceinline
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
          res, \
          (ckd_uintmax)(a), \
          (ckd_uintmax)(b), \
          (ckd_is_signed(a) << 1) | ckd_is_signed(b)))

#    define ckd_declare_add(S, T) \
      ckd_inline char S( \
          void* res, ckd_uintmax x, ckd_uintmax y, char ab_signed) \
      { \
        ckd_uintmax z = x + y; \
        *(T*)res = (T)z; \
        char truncated = 0; \
        if (sizeof(T) < sizeof(ckd_intmax)) { \
          truncated = z != (ckd_uintmax)(T)z; \
        } \
        switch (ckd_is_signed((T)0) << 2 | ab_signed) { \
          case 0: /* u = u + u */ \
            return (char)(truncated | (z < x)); \
          case 1: /* u = u + s */ \
            y ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | ((ckd_intmax)((z ^ x) & (z ^ y)) < 0)); \
          case 2: /* u = s + u */ \
            x ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | ((ckd_intmax)((z ^ x) & (z ^ y)) < 0)); \
          case 3: /* u = s + s */ \
            return ( \
                char)(truncated \
                      | ((ckd_intmax)(((z | x) & y) | ((z & x) & ~y)) < 0)); \
          case 4: /* s = u + u */ \
            return (char)(truncated | (z < x) | ((ckd_intmax)z < 0)); \
          case 5: /* s = u + s */ \
            y ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | (x + y < y)); \
          case 6: /* s = s + u */ \
            x ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | (x + y < x)); \
          case 7: /* s = s + s */ \
            return (char)(truncated | ((ckd_intmax)((z ^ x) & (z ^ y)) < 0)); \
          default: \
            ckd_unreachable(0); \
        } \
      }

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

#    define ckd_declare_sub(S, T) \
      ckd_inline char S( \
          void* res, ckd_uintmax x, ckd_uintmax y, char ab_signed) \
      { \
        ckd_uintmax z = x - y; \
        *(T*)res = (T)z; \
        char truncated = 0; \
        if (sizeof(T) < sizeof(ckd_intmax)) { \
          truncated = z != (ckd_uintmax)(T)z; \
        } \
        switch (ckd_is_signed((T)0) << 2 | ab_signed) { \
          case 0: /* u = u - u */ \
            return (char)(truncated | (x < y)); \
          case 1: /* u = u - s */ \
            y ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | ((ckd_intmax)((x ^ y) & (z ^ x)) < 0)); \
          case 2: /* u = s - u */ \
            return (char)(truncated | (y > x) | ((ckd_intmax)x < 0)); \
          case 3: /* u = s - s */ \
            return ( \
                char)(truncated \
                      | ((ckd_intmax)(((z & x) & y) | ((z | x) & ~y)) < 0)); \
          case 4: /* s = u - u */ \
            return (char)(truncated | ((x < y) ^ ((ckd_intmax)z < 0))); \
          case 5: /* s = u - s */ \
            y ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | (x >= y)); \
          case 6: /* s = s - u */ \
            x ^= ckd_sign(ckd_uintmax); \
            return (char)(truncated | (x < y)); \
          case 7: /* s = s - s */ \
            return (char)(truncated | ((ckd_intmax)((x ^ y) & (z ^ x)) < 0)); \
          default: \
            ckd_unreachable(0); \
        } \
      }

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

#    define ckd_declare_mul(S, T) \
      ckd_inline char S( \
          void* res, ckd_uintmax x, ckd_uintmax y, char ab_signed) \
      { \
        switch (ckd_is_signed((T)0) << 2 | ab_signed) { \
          case 0: { /* u = u * u */ \
            ckd_uintmax z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = (T)z; \
            return (char)(o \
                          | (sizeof(T) < sizeof(z) \
                             && z != (ckd_uintmax)(*(T*)res))); \
          } \
          case 1: { /* u = u * s */ \
            ckd_uintmax z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = (T)z; \
            return (char)(( \
                o | (((ckd_intmax)y < 0) & !!x) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax)(*(T*)res)))); \
          } \
          case 2: { /* u = s * u */ \
            ckd_uintmax z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = (T)z; \
            return (char)(( \
                o | (((ckd_intmax)x < 0) & !!y) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax)(*(T*)res)))); \
          } \
          case 3: { /* u = s * s */ \
            int o = 0; \
            if ((ckd_intmax)(x & y) < 0) { \
              x = -x; \
              y = -y; \
            } else if ((ckd_intmax)(x ^ y) < 0) { \
              o = x && y; \
            } \
            ckd_uintmax z = x * y; \
            o |= x && z / x != y; \
            *(T*)res = (T)z; \
            return (char)(o \
                          | (sizeof(T) < sizeof(z) \
                             && z != (ckd_uintmax)(*(T*)res))); \
          } \
          case 4: { /* s = u * u */ \
            ckd_uintmax z = x * y; \
            int o = x && z / x != y; \
            *(T*)res = (T)z; \
            return (char)(( \
                o | ((ckd_intmax)(z) < 0) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax)(*(T*)res)))); \
          } \
          case 5: { /* s = u * s */ \
            ckd_uintmax t = -y; \
            t = (ckd_intmax)(t) < 0 ? y : t; \
            ckd_uintmax p = t * x; \
            int o = t && p / t != x; \
            int n = (ckd_intmax)y < 0; \
            ckd_uintmax z = n ? -p : p; \
            *(T*)res = (T)z; \
            ckd_uintmax m = ckd_sign(ckd_uintmax) - 1; \
            return (char)(( \
                o | (p > m + (ckd_uintmax)n) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax)(*(T*)res)))); \
          } \
          case 6: { /* s = s * u */ \
            ckd_uintmax t = -x; \
            t = (ckd_intmax)(t) < 0 ? x : t; \
            ckd_uintmax p = t * y; \
            int o = t && p / t != y; \
            int n = (ckd_intmax)x < 0; \
            ckd_uintmax z = n ? -p : p; \
            *(T*)res = (T)z; \
            ckd_uintmax m = ckd_sign(ckd_uintmax) - 1; \
            return (char)(( \
                o | (p > m + (ckd_uintmax)n) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax)(*(T*)res)))); \
          } \
          case 7: { /* s = s * s */ \
            ckd_uintmax z = x * y; \
            *(T*)res = (T)z; \
            return (char)(( \
                ((((ckd_intmax)y < 0) && (x == ckd_sign(ckd_uintmax))) \
                 || (y && (((ckd_intmax)z / (ckd_intmax)y) != (ckd_intmax)x))) \
                | (sizeof(T) < sizeof(z) && z != (ckd_uintmax)(*(T*)res)))); \
          } \
          default: \
            ckd_unreachable(0); \
        } \
      }

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

#  else
#    pragma message( \
        "checked integer arithmetic unsupported in this environment")

#    define ckd_add(res, x, y) (*(res) = (x) + (y), 0)
#    define ckd_sub(res, x, y) (*(res) = (x) - (y), 0)
#    define ckd_mul(res, x, y) (*(res) = (x) * (y), 0)

#  endif /* GNU */
#endif /* stdckdint.h */
#endif /* JTCKDINT_H_ */
