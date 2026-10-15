#pragma once

/**
 * @file FakePort.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/ipc/Port.hpp>

namespace mock
{
    class FakePort : public npos::ipc::Port
    {
    public:
        explicit FakePort(npos::ipc::Message::Type acceptedType = 0x1000)
            : acceptedType(acceptedType)
        {
        }

        bool accepts(npos::ipc::Message::Type type) const override { return type == acceptedType; }
        void receiveRequest(const npos::ipc::Message& message) override { received.push_back(&message); }
        void receiveResponse(const npos::ipc::Message& message) override { receivedResponses.push_back(&message); }
        void handleUnknownRequest(npos::ipc::Message& message) override { unknown.push_back(&message); }

        npos::ipc::Message::Type acceptedType;
        etl::vector<const npos::ipc::Message*, 16> received;
        etl::vector<const npos::ipc::Message*, 16> receivedResponses;
        etl::vector<const npos::ipc::Message*, 16> unknown;
    };
}
