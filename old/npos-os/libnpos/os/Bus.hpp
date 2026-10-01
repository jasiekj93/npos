#pragma once

/**
 * @file Bus.hpp
 * @author Adrian Szczepanski
 * @date 25-09-2026
 */

#include <etl/priority_queue.h>

#include <libnpos/os/Message.hpp>
#include <libnpos/os/Task.hpp>

namespace npos::os
{
    class Bus
    {
    public:
        virtual ~Bus() = default;

        virtual void sendTo(Pid recipient, const Message&) = 0;
        virtual void broadcast(const Message&) = 0;
        virtual void respond(const Message&) = 0;
        virtual void release(const Message const*) = 0;
    };
}