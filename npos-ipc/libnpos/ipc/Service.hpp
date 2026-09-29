#pragma once

/**
 * @file Service.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc
{
    class Service
    {
    public:
        class Api
        {
        public:
            virtual ~Api() = default;

            virtual void send(Message& message) = 0;
            virtual void respond(Message& message) = 0;
            virtual PortId getPortId() const = 0;
        };

        explicit Service(Api& api) : api(api) {}
        virtual ~Service() = default;

        virtual void handle(Message&) = 0;
        virtual bool accepts(Message::Type) const = 0;

        inline void setId(ServiceId id) { serviceId = id; }
        inline Api& getApi() const { return api; }

    protected:
        inline ServiceId getId() const { return serviceId; }

        Api& api;

    private:
        ServiceId serviceId = NULL_SERVICE;
    };
}