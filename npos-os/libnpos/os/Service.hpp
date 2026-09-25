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

        virtual void onReceive(const Message& message) = 0;
        virtual bool accepts(Message::Type) const = 0;

        inline void setId(Pid newId) { id = newId; }
        inline auto getId() const { return id; }
        inline auto& getBus() const { return bus; }

    protected:
        void sendTo(Pid recipient, os::Message& message)
        {
            message.sender = id;
            bus.publish(recipient, message);
        }

        void broadcast(os::Message& message)
        {
            message.sender = id;
            bus.publish(message);
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