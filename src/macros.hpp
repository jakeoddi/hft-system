#pragma once

#include <iostream>

#define LIKELY(x) __builtin_expect(!!(x), 1) // double negation ensures explicit boolean conversion
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

inline void ASSERT(bool condition, const std::string& message) {
    if (UNLIKELY(!condition)) {
        std::cerr << message << std::endl;
        exit(EXIT_FAILURE);
    }
}

inline void FATAL(const std::string& message) {
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}