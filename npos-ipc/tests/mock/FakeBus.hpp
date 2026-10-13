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
        void send(npos::ipc::Message& message) override { sent.push_back(&message); }
        void respond(npos::ipc::Message& message) override { responded.push_back(&message); }
        void release(npos::ipc::Message* message) override { released.push_back(message); }

        etl::vector<npos::ipc::Message*, 16> sent;
        etl::vector<npos::ipc::Message*, 16> responded;
        etl::vector<npos::ipc::Message*, 16> released;
    };
}
