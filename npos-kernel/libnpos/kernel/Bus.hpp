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

        virtual void sendTo(Pid recipient, const Message&) = 0;
        virtual void broadcast(const Message&) = 0;
        virtual void respond(const Message&) = 0;
        virtual void release(const Message* const) = 0;
    };
}