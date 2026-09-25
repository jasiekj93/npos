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
        static constexpr Pid BROADCAST = 255;

        virtual ~Bus() = default;

        virtual void publish(Pid recipient, const Message& message) = 0;
        virtual void publish(const Message& message) = 0;
        virtual void release(const Message& message) = 0;
    };


   
}