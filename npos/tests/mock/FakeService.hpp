#pragma once

/**
 * @file FakeService.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/ipc/Service.hpp>

namespace mock
{
    class FakeService : public npos::ipc::Service
    {
    public:
        explicit FakeService(Api& api, npos::ipc::Message::Type acceptedType = 0x1000)
            : npos::ipc::Service(api)
            , acceptedType(acceptedType)
        {
        }

        void initialize() override { initialized++; }
        void handleRequest(npos::ipc::Message& message) override { handled.push_back(&message); }
        void handleResponse(npos::ipc::Message& message) override { handledResponses.push_back(&message); }
        void handleUnknownRequest(npos::ipc::Message& message) override
        {
            handledUnknown.push_back(&message);
            npos::ipc::Service::handleUnknownRequest(message);
        }
        bool acceptsRequest(npos::ipc::Message::Type type) const override { return type == acceptedType; }

        npos::ipc::Message::Type acceptedType;
        int initialized = 0;
        etl::vector<npos::ipc::Message*, 16> handled;
        etl::vector<npos::ipc::Message*, 16> handledResponses;
        etl::vector<npos::ipc::Message*, 16> handledUnknown;
    };
}
