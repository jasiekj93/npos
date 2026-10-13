#pragma once

/**
 * @file Type.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg
{
    enum Type : ipc::Message::Type
    {
        FILE_OPEN = ipc::FILESYSTEM_MESSAGE_TYPE,
        FILE_CLOSE,
        FILE_READ,
        FILE_WRITE,
        DIR_OPEN,
        DIR_CLOSE,
        DIR_READ,
    };
}