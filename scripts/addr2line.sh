#!/bin/sh
set -e
if [ $# -eq 0 ]; then
    echo "Usage: $0 reloc_addr debug_file reloc_base"
    exit 1
fi
reloc_addr=$1
file=${2:-build/Debug/bin/DashFaction.dll.debug}
reloc_base=${3:-0}
#file=build/Debug/bin/DashFaction.dll.debug
img_base=$(i686-w64-mingw32-objdump -t "$file" | grep __image_base__ | grep -oE '0x[0-9A-F]{8}')
echo "Image base: $img_base"
echo "Image test: $((img_base))"
img_addr=$(printf "0x%X" $((reloc_addr - reloc_base + img_base)))
echo "Address in image: $img_addr"
i686-w64-mingw32-addr2line -e "$file" $img_addr
