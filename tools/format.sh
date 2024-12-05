#!/bin/sh
CLANG_FORMAT_FLAGS="${CLANG_FORMAT_FLAGS:--i}"
find -regex './\(vendor\|build\|research\|\..*\)' -prune -o -regex '.*\.\(cpp\|h\|hpp\|cc\|cxx\)' \
  -exec clang-format $CLANG_FORMAT_FLAGS \{} \;
