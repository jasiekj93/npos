#pragma once

/**
 * @file SystemBus.hpp
 * @author Adrian Szczepanski
 * @date 18-09-2026
 */

#include <etl/vector.h>
#include <etl/pool.h>

#include <libnpos/os/Message.hpp>
#include <libnpos/os/Service.hpp>

namespace npos::os
{
    class SystemBus : public Bus
    {
    public:
        using ServiceList = etl::ivector<Service*>;
        using MessagePool = etl::ipool;

        SystemBus(Pid startIndex, ServiceList& services, MessagePool& messages);

        bool subscribe(Service&);
        inline bool full() const { return services.full(); }

        void sendTo(Pid recipient, const Message&) override;
        void broadcast(const Message&) override;
        void respond(const Message&) override;
        void release(const Message const*) override;

        inline void setSuccessor(SystemBus* next) { successor = next; }

    protected:
        size_t getIndex(Pid pid) const;
        Pid getServiceId(size_t index) const;

    private:
        Pid startId;
        ServiceList& services;
        MessagePool& messagePool;
        SystemBus* successor;
    };
}