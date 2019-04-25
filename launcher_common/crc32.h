#pragma once

#include <cstdint>
#include <fstream>

//extern "C" {
uint32_t crc32(uint32_t crc, const void *bufp, size_t len);
//}

inline uint32_t GetFileCRC32(const char *path)
{
    char buf[4096];
    std::ifstream file(path, std::fstream::in | std::fstream::binary);
    if (!file) return 0;
    uint32_t hash = 0;
    while (file)
    {
        file.read(buf, sizeof(buf));
        size_t len = file.gcount();
        if (!len) break;
        hash = crc32(hash, buf, len);
    }
    return hash;
}
