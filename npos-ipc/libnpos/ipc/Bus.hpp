#pragma once

/**
 * @file Bus.hpp
 * @author Adrian Szczepanski
 * @date 25-09-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc
{
    class Bus
    {
    public:
        virtual ~Bus() = default;

        virtual void send(Message&) = 0;
        virtual void respond(Message&) = 0;
    };
}