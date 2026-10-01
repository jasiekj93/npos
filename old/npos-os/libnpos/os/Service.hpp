#pragma once

/**
 * @file Service.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/Bus.hpp>
#include <libnpos/os/syslog/Stream.hpp>

namespace npos::os
{
    class Service
    {
    public:
        explicit Service(Pid id, Bus& bus) 
            : bus(bus), id(id) {}

        virtual ~Service() = default;

        virtual void initalize() {}

        virtual void receive(Message& message)
        {
            onReceive(message);
            bus.release(&message);
        }

        virtual bool accepts(Message::Type) const = 0;

        inline void setId(Pid newId) { id = newId; }
        inline auto getId() const { return id; }
        inline auto& getBus() const { return bus; }

    protected:
        virtual void onReceive(Message&) = 0;

        void sendTo(Pid recipient, os::Message& message)
        {
            message.sender = id;
            bus.sendTo(recipient, message);
        }

        void broadcast(os::Message& message)
        {
            message.sender = id;
            bus.broadcast(message);
        }

        void respond(os::Message& message)
        {
            bus.respond(message);
        }

        syslog::Stream syslog(syslog::Level level)
        {
            return syslog::log(id, level, bus);
        }

    private:
        Bus& bus;
        Pid id;
    };
}