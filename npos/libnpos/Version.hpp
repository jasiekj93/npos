#pragma once

/**
 * @file Version.hpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include <cstdint>

namespace npos
{
    struct Version
    {
        using Value = uint32_t;
        using Major = uint8_t;
        using Minor = uint8_t;
        using Patch = uint8_t;

        union 
        {
            struct
            {
                Major major;
                Minor minor;
                Patch patch;
            };

            Value value;
        };
    };

    constexpr Version::Major VERSION_MAJOR = 0;
    constexpr Version::Minor VERSION_MINOR = 1;
    constexpr Version::Patch VERSION_PATCH = 0;

    constexpr Version VERSION = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH };
}