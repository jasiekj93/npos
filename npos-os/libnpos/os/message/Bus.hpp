#pragma once

/**
 * @file Bus.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <cstddef>
#include <etl/vector.h>

#include <libnpos/ipc/MessageRouter.hpp>

namespace npos::os::message
{
    class Bus : public ipc::MessageRouter
    {
    public:
        static constexpr size_t SIZE = 255;

        Bus(); 

        bool subscribe(ipc::MessageRouter&);

        void receive(const ipc::Message& message) override;
        void receive(ipc::MessageRouter::Id destinationId, const ipc::Message& message) override;

        inline void onReceive(const ipc::Message&) override {}
        inline bool accepts(ipc::Message::Id) const override { return true; }

    private:
        etl::vector<ipc::MessageRouter*, SIZE> routerList;
    };
}