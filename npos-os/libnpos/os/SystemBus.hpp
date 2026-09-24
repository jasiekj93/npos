#pragma once

/**
 * @file SystemBus.hpp
 * @author Adrian Szczepanski
 * @date 18-09-2026
 */

#include <etl/vector.h>

#include <libnpos/os/Message.hpp>
#include <libnpos/os/Service.hpp>

namespace npos::os
{
    class SystemBus : public ipc::MessageRouter
    {
    public:
        using ServiceList = etl::ivector<Service*>;

        SystemBus(Service::Id startIndex, ServiceList& services);

        bool subscribe(Service&);
        inline bool full() const { return services.full(); }

        void receive(ipc::MessageRouter::Id recipient, const ipc::Message&) override;
        void receive(const ipc::Message&) override;

        inline void setSuccessor(SystemBus* next) { successor = next; }

    protected:
        size_t getIndex(Service::Id pid) const;
        Service::Id getServiceId(size_t index) const;

    private:
        Service::Id startId;
        ServiceList& services;
        SystemBus* successor;
    };
}