#!/bin/sh
set -e

clang_tidy=clang-tidy-11
cc=i686-w64-mingw32-gcc-posix
compile_commands=build/Release/compile_commands.json
target=i686-w64-mingw32-posix

include_dirs=$(echo | $cc -xc++ -E -v - 2>&1 | sed '/#include <...> search starts here/,/End of search list/!d;//d')
isystem_flags=$(echo "$include_dirs" | sed -e 's/^\s*/-extra-arg-before=-isystem -extra-arg-before=/')

# -header-filter='main/.*'
# -extra-arg-before=-v

#echo $clang_tidy -p $compile_commands -extra-arg-before=--target=$target $isystem_flags $@
$clang_tidy -p $compile_commands -extra-arg-before=--target=$target $isystem_flags $@
