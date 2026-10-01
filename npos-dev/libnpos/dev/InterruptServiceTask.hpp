#pragma once

/**
 * @file InterruptServiceTask.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/dev/InterruptService.hpp>

namespace npos::dev
{
    class InterruptServiceTask : public ipc::ServiceTask
    {
    public:
        using InterruptServiceListInt = etl::ivector<InterruptService*>;

        template<size_t N>
        using InterruptServiceList = etl::vector<InterruptService*, N>;

        InterruptServiceTask(Priority priority, InterruptServiceListInt& services, MessageQueueInt& messageQueue, ipc::Bus& bus)
            : ipc::ServiceTask(priority, reinterpret_cast<ServiceListInt&>(services), messageQueue, bus)
        {
        }

        bool isReady() const override
        {
            for(auto& service : getServices())
            {
                auto& interruptService = static_cast<InterruptService&>(*service);

                if(interruptService.isInterruptPending())
                    return true;
            }

            return ServiceTask::isReady();
        }

        void process() override
        {
            for(auto& service : getServices())
            {
                auto& interruptService = static_cast<InterruptService&>(*service);

                if(interruptService.isInterruptPending())
                    return interruptService.processInterrupt();
            }

            return ServiceTask::process();
        }
    };
}