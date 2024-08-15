#include "jtckdint.h"

bool test_odr(int a, int b);

bool test_odr(int a, int b)
{
  int c;
  return !ckd_add(&c, a, b) && !ckd_sub(&c, a, b) && !ckd_mul(&c, a, b);
}
