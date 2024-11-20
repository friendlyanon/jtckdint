#!/bin/sh
set -ex

for cc in clang cc; do
  for opt in -O0 -O3 -fsanitize=undefined; do
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror $opt"
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror -pedantic-errors $opt -std=c11"
  done
done

for cc in c++ clang++; do
  for opt in -O0 -O3 -fsanitize=undefined; do
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror $opt" CFLAGS="-xc++"
    make clean
    make CC="$cc -Wall -Wextra -Wno-parentheses -Werror -pedantic-errors $opt -std=c++11" CFLAGS="-xc++"
  done
done

make clean
make
