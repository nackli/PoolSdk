#pragma once
#include <cstdint>
template<typename T>
uint32_t javaHashCode(const T& container) {
    uint32_t hash = 0;
    for (const auto& element : container) 
        hash = 31 * hash + static_cast<uint32_t>(element);
    return hash;
}

uint32_t ELFhash(const char* key)
{
    uint32_t h = 0;
    while (*key)
    {
        h = (h << 4) + *key++;
        uint32_t g = h & 0xF0000000L;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}