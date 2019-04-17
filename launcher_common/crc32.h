#pragma once

#include <cstdint>
#include <cstdio>

//extern "C" {
uint32_t crc32(uint32_t crc, const void *bufp, size_t len);
//}

inline uint32_t GetFileCRC32(const char *path)
{
    char buf[1024];
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    uint32_t hash = 0;
    while (1)
    {
        size_t len = fread(buf, 1, sizeof(buf), file);
        if (!len) break;
        hash = crc32(hash, buf, len);
    }
    fclose(file);
    return hash;
}
