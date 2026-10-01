#pragma once

/**
 * @file Id.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <libnpos/os/Message.hpp>

namespace npos::os::filesystem::message
{
    enum Id : os::Message::Type
    {
        FILE_OPEN = os::FILESYSTEM_MESSAGE_TYPE,
        FILE_CLOSE,
        FILE_WRITE,
        FILE_READ,
        FILE_SYNC,
        FILE_SEEK,
        FILE_TRUNCATE,
        FILE_TELL,

        DIR_MAKE,
        DIR_OPEN,
        DIR_CLOSE,
        DIR_READ,
        DIR_SEEK,
        DIR_TELL,

        SYSTEM_FORMAT,
        SYSTEM_MOUNT,
        SYSTEM_UNMOUNT,
        SYSTEM_REMOVE,
        SYSTEM_RENAME,
        SYSTEM_STATFS,
    };
}