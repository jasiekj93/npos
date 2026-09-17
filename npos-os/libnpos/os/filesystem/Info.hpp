#pragma once

/**
 * @file Info.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <cstdint>
#include <cstddef>

#include <libnpos/os/filesystem/Path.hpp>

namespace npos::os::filesystem
{
    struct Info 
    {
        enum Type
        {
            FILE,
            DIRECTORY
        };

        Type type;

        // Size of the file, only valid for files
        size_t size;

        Path name;
    };
}