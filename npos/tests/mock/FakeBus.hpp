#pragma once

/**
 * @file FakeBus.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/ipc/Bus.hpp>

namespace mock
{
    class FakeBus : public npos::ipc::Bus
    {
    public:
        // Responses (isResponse) are recorded separately from requests.
        void send(npos::ipc::Message& message) override
        {
            if(message.isResponse)
                responded.push_back(&message);
            else
                sent.push_back(&message);
        }
        void release(npos::ipc::Message* message) override { released.push_back(message); }

        etl::vector<npos::ipc::Message*, 16> sent;
        etl::vector<npos::ipc::Message*, 16> responded;
        etl::vector<npos::ipc::Message*, 16> released;
    };
}
