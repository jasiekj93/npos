#pragma once

/**
 * @file Info.hpp
 * @author Adrian Szczepanski
 * @date 06-11-2026
 */

#include <cstdint>
#include <etl/string_view.h>
#include <libnpos/fs/utils/String.hpp>

namespace npos::fs
{
    struct Info
    {
        static constexpr size_t NAME_SIZE = 64;

        enum Type : uint8_t
        {
            FILE,
            DIRECTORY
        };

        Type type = FILE;
        uint32_t size = 0;
        char name[NAME_SIZE] = {};

        inline etl::string_view getName() const
        {
            return etl::string_view(name, utils::strnlen(name, NAME_SIZE));
        }
    };
}