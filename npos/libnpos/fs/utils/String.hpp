#pragma once

/**
 * @file String.hpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include <cstddef>
#include <cstdint>

namespace npos::fs::utils
{
    constexpr size_t strnlen(const char* str, size_t maxLength)
    {
        size_t length = 0;

        while (length < maxLength and str[length] != '\0')
        {
            ++length;
        }

        return length;
    } 
}