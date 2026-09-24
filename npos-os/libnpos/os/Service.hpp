#pragma once

/**
 * @file Service.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/Message.hpp>
#include <libnpos/os/syslog/Stream.hpp>

namespace npos::os
{
    class Service : public ipc::MessageRouter
    {
    public:
        using Id = ipc::MessageRouter::Id;
        using SystemBus = ipc::MessageRouter;

        explicit Service(SystemBus& bus) 
            : ipc::MessageRouter(0)
            , bus(bus) {}

        virtual ~Service() = default;

        virtual void initalize() {}

        // virtual void onReceive(const Message&) = 0;
        // virtual bool accepts(Message::Id) const = 0;

    protected:
        void sendTo(Id recipient, os::Message& message)
        {
            message.senderId = getId();
            bus.receive(recipient, message);
        }

        void broadcast(os::Message& message)
        {
            message.senderId = getId();
            bus.receive(message);
        }

        syslog::Stream syslog(syslog::Level level)
        {
            return syslog::log(getId(), level, bus);
        }

    private:
        SystemBus& bus;
    };
}