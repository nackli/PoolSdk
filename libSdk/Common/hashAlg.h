#pragma once
#include <cstdint>
template<typename T>
uint32_t javaHashCode(const T& container) {
    uint32_t hash = 0;
    for (const auto& element : container) 
        hash = 31 * hash + static_cast<uint32_t>(element);
    return hash;
}

