#pragma once

/**
 * @file Type.hpp
 * @author Adrian Szczepanski
 * @date 14-10-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::cli::msg
{
    enum Type : ipc::Message::Type
    {
        SYSLOG = ipc::CLI_MESSAGE_TYPE,
        REGISTER_NAME,
    };
}