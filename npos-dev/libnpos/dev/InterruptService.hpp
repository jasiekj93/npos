#pragma once

/**
 * @file InterruptService.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <etl/optional.h>
#include <etl/string_view.h>
#include <etl/utility.h>
#include <etl/vector.h>
#include <etl/queue.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/hal/Interrupt.hpp>

namespace npos::dev
{
    class InterruptService : public ipc::Service, public hal::InterruptHandler
    {
    public:
        using InterruptQueueInt = etl::iqueue<hal::Interrupt>;

        template<size_t N>
        using InterruptQueue = etl::queue<hal::Interrupt, N>;

        InterruptService(Api& api, InterruptQueueInt& interruptQueue, hal::Interruptable& device)
            : ipc::Service(api)
            , interruptQueue(interruptQueue)
            , device(device)
        {
            device.setHandler(this);
        }

        virtual void onInterrupt(const hal::Interrupt& interrupt) = 0;

        //from ISR
        void handleInterrupt(const hal::Interrupt& interrupt) override
        {
            if(interruptQueue.full())
                return; //TODO handle

            interruptQueue.push(interrupt);
        }

        bool isInterruptPending() const
        {
            bool ready = false;

            device.disableInterrupts();
            ready = (not interruptQueue.empty());
            device.enableInterrupts();

            return ready;
        }

        void processInterrupt()
        {
            device.disableInterrupts();
            if(interruptQueue.empty())
            {
                device.enableInterrupts();
                return;
            }

            hal::Interrupt interrupt = interruptQueue.front();
            interruptQueue.pop();
            device.enableInterrupts();

            onInterrupt(interrupt);
        }

    private:
        hal::Interruptable& device;
        InterruptQueueInt& interruptQueue;
    };
}