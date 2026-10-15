#pragma once

/**
 * @file Probe.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <functional>

#include <libnpos/ipc/Service.hpp>

namespace mock
{
    // User service whose behaviour is given by the test.
    class Probe : public npos::ipc::Service
    {
    public:
        explicit Probe(Api& api) : npos::ipc::Service(api) {}

        void initialize() override
        {
            if(onInitialize)
                onInitialize(*this);
        }

        void handleRequest(npos::ipc::Message& message) override
        {
            messages++;

            if(onMessage)
                onMessage(*this, message);
        }

        void handleResponse(npos::ipc::Message& message) override
        {
            responses++;

            if(onResponse)
                onResponse(*this, message);
        }

        bool acceptsRequest(npos::ipc::Message::Type) const override { return true; }

        std::function<void(Probe&)> onInitialize;
        std::function<void(Probe&, npos::ipc::Message&)> onMessage;
        std::function<void(Probe&, npos::ipc::Message&)> onResponse;
        int messages = 0;
        int responses = 0;
    };
}
