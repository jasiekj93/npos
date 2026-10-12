#pragma once

/**
 * @file Flags.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/initializer_list.h>

#include <libnpos/ipc/Message.hpp>

namespace npos::fs
{
    enum Flag : ipc::Message::Input::Open::Flags
    {
        READ_ONLY = 0x01,
        WRITE_ONLY = 0x02,
        READ_WRITE = 0x03,
        CREATE = 0x0100,
        EXCLUSIVE = 0x0200,
        TRUNCATE = 0x0400,
        APPEND = 0x0800,
    };

    using Flags = etl::initializer_list<Flag>;
}