#pragma once

/**
 * @file Path.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/string.h>

#include <libnpos/kernel/Message.hpp>

namespace npos::fs
{
    static constexpr size_t MAX_PATH_LENGTH = kernel::Message::PAYLOAD_SIZE / 2;

    using Path = etl::string<MAX_PATH_LENGTH>;
}