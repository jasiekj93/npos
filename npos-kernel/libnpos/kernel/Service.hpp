#pragma once

/**
 * @file Service.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/kernel/Bus.hpp>
#include <libnpos/kernel/syslog/Stream.hpp>

namespace npos::kernel
{
    class Service
    {
    public:
        explicit Service(Bus& bus) 
            : bus(bus), id(0) {}

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

        void sendTo(Pid recipient, Message& message)
        {
            message.sender = id;
            bus.sendTo(recipient, message);
        }

        void broadcast(Message& message)
        {
            message.sender = id;
            bus.broadcast(message);
        }

        void respond(Message& message)
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