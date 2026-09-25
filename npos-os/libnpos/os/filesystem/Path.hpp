#pragma once

/**
 * @file Path.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/string.h>

#include <libnpos/os/Message.hpp>

namespace npos::os::filesystem
{
    static constexpr size_t MAX_PATH_LENGTH = os::Message::PAYLOAD_SIZE / 2;

    using Path = etl::string<MAX_PATH_LENGTH>;
}