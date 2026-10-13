#pragma once

/**
 * @file Service.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/ipc/Log.hpp>

namespace npos::ipc
{
    class Service
    {
    public:
        class Api : public log::Stream::Transmitter
        {
        public:
            virtual ~Api() = default;

            virtual void send(Message& message) = 0;
            virtual void respond(Message& message) = 0;
            virtual PortId getPortId() const = 0;

            void sendSyslog(log::Syslog& message) override { send(message); }
        };

        explicit Service(Api& api) : api(api) {}
        virtual ~Service() = default;

        virtual void initialize() {}
        virtual void handle(Message&) = 0;
        virtual bool accepts(Message::Type) const = 0;

        inline void setId(ServiceId id) { serviceId = id; }
        inline Api& getApi() const { return api; }
        inline ipc::Oid getOid() const { return { api.getPortId(), serviceId }; }
        inline log::Stream log(log::Level level) { return log::Stream(level, getOid(), api); }

    protected:
        Api& api;

    private:
        ServiceId serviceId = NULL_SERVICE;
    };
}