#!/bin/sh
set -e

self_dir=$(dirname "$(readlink -f "$0")")
clang_tidy="$self_dir/clang-tidy.sh"

src_dir=${1:-.}

find "$src_dir" -type f -iname '*.cpp' \
    -not -path '*/experimental*' \
    -not -path '*/vendor/*' \
    -not -path '*/research/*' \
    -print0 | xargs -0 -L1 -P4 "$clang_tidy"
