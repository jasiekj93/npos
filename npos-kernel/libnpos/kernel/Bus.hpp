#pragma once

/**
 * @file Bus.hpp
 * @author Adrian Szczepanski
 * @date 25-09-2026
 */

#include <libnpos/kernel/Message.hpp>

namespace npos::kernel
{
    class Bus
    {
    public:
        virtual ~Bus() = default;

        virtual void sendTo(Pid recipient, Message&) = 0;
        virtual void broadcast(Message&) = 0;
        virtual void respond(Message&) = 0;
        virtual void release(const Message* const) = 0;
    };
}