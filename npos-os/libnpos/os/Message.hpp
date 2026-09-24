#pragma once

/**
 * @file Message.hpp
 * @author Adrian Szczepanski
 * @date 18-09-2026
 */

#include <libnpos/ipc/MessageRouter.hpp>

namespace npos::os
{
    class Message : public ipc::Message
    {
    public:
        using Type = ipc::Message::Id;
        using SenderId = ipc::MessageRouter::Id;

        static constexpr Type OS_ERROR = 0x0000;
        static constexpr Type OS_SYSLOG = 0x0100;
        static constexpr Type DEVICE = 0x0200;
        static constexpr Type DRIVER = 0x0300;
        static constexpr Type FILESYSTEM = 0x0400;
        static constexpr Type CLI = 0x0500;
        static constexpr Type USER = 0x0600;

        Message(Type type)
            : ipc::Message(static_cast<ipc::Message::Id>(type))
        {
        }

        SenderId senderId;

        inline Type getType() const
        {
            return static_cast<Type>(ipc::Message::getId());
        }

    };
}