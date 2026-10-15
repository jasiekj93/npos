#pragma once

/**
 * @file RecordingApi.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/ipc/Service.hpp>

namespace mock
{
    // Service::Api that only remembers what was sent and responded.
    class RecordingApi : public npos::ipc::Service::Api
    {
    public:
        void send(npos::ipc::Service& sender, npos::ipc::Message& message) override
        {
            message.sender = sender.getOid();
            message.isResponse = false;
            sent.push_back(message);
        }
        void respond(npos::ipc::Message& message) override
        {
            message.isResponse = true;
            responded.push_back(message);
        }
        npos::ipc::PortId getPortId() const override { return portId; }

        npos::ipc::PortId portId = 3;
        etl::vector<npos::ipc::Message, 16> sent;
        etl::vector<npos::ipc::Message, 16> responded;
    };
}
