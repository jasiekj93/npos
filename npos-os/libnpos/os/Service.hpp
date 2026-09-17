#pragma once

/**
 * @file Service.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/ipc/MessageRouter.hpp>
#include <libnpos/os/message/Bus.hpp>

namespace npos::os
{
    class Service : public ipc::MessageRouter
    {
    public:
        explicit Service(message::Bus& bus) 
            : ipc::MessageRouter(0)
            , bus(bus) {}

        virtual ~Service() = default;

        virtual void initalize() {}

        // virtual void onReceive(const Message&) = 0;
        // virtual bool accepts(Message::Id) const = 0;

    protected:
        message::Bus& bus;
    };
}