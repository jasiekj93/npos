#pragma once

/**
 * @file UserApi.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/ipc/Service.hpp>

namespace npos::os
{
    // Lets user services be created before the system that runs them; System binds it to the user task.
    class UserApi : public ipc::Service::Api, public ipc::log::Stream::Transmitter
    {
    public:
        inline void bind(ipc::Service::Api& target) { api = &target; }
        inline bool isBound() const { return api != nullptr; }

        void send(ipc::Message& message) override
        {
            if(api)
                api->send(message);
        }

        void respond(ipc::Message& message) override
        {
            if(api)
                api->respond(message);
        }

        ipc::PortId getPortId() const override
        {
            return api ? api->getPortId() : ipc::NULL_PORT;
        }

        ipc::log::Stream log(ipc::log::Level level, ipc::ServiceId serviceId) override
        {
            return ipc::log::Stream(level, { getPortId(), serviceId }, *this);
        }

        void sendSyslog(ipc::log::Syslog& message) override
        {
            // if(api)
            //     api->sendSyslog(message);
            //TODO
        }

    private:
        ipc::Service::Api* api = nullptr;
    };
}
