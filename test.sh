#!/bin/sh
set -ex

for cc in clang cc; do
  for opt in -O0 -O3 -fsanitize=undefined; do
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror -DJTCKDINT_OPTION_STDCKDINT=2 $opt"
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror -pedantic-errors -DJTCKDINT_OPTION_STDCKDINT=2 $opt -std=c11"
  done
done

for cc in c++ clang++; do
  for opt in -O0 -O3 -fsanitize=undefined; do
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror -DJTCKDINT_OPTION_STDCKDINT=2 $opt" CFLAGS="-xc++"
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror -pedantic-errors -DJTCKDINT_OPTION_STDCKDINT=2 $opt -std=c++11" CFLAGS="-xc++"
  done
done

make clean
make
