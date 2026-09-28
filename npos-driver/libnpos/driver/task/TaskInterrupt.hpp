#pragma once

/**
 * @file TaskInterrupt.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <etl/queue.h>

#include <libnpos/kernel/Task.hpp>
#include <libnpos/driver/hal/Interrupt.hpp>

namespace npos::driver::task
{
    class TaskInterrupt : public kernel::ServiceTask, public hal::InterruptHandler
    {
    public:
        using InterruptQueue = etl::iqueue<hal::Interrupt>;

        TaskInterrupt(Priority priority, MessageQueue& messageQueue, Service& service, 
            hal::Interruptable& device, hal::InterruptHandler& interruptService, InterruptQueue& interruptQueue)
            : ServiceTask(priority, messageQueue, service)
            , device(device)
            , interruptService(interruptService)
            , interruptQueue(interruptQueue) {}

        virtual ~TaskInterrupt() = default;

        void handleInterrupt(const hal::Interrupt& interrupt) override
        {
            //on ISR
            if(not interruptQueue.full())
                interruptQueue.emplace(interrupt);
        }

        void initalize() override 
        { 
            ServiceTask::initalize(); 
            device.setHandler(this);
        } 

        bool isReady() const override 
        { 
            bool ready = false;
            device.disableInterrupts();
            ready = not interruptQueue.empty();
            device.enableInterrupts();

            if(not ready)
                return ServiceTask::isReady();
            else
                return true;
        }

        void process() override
        { 
            bool handled = false;
            hal::Interrupt interrupt;

            device.disableInterrupts();
            if(not interruptQueue.empty())
            {
                interrupt = interruptQueue.front();
                interruptQueue.pop();
                handled = true;
            }
            device.enableInterrupts();

            if(handled)
                interruptService.handleInterrupt(interrupt);
            else
                return ServiceTask::process();
        }

    private:
        hal::Interruptable& device;
        hal::InterruptHandler& interruptService;
        InterruptQueue& interruptQueue;
    };

}