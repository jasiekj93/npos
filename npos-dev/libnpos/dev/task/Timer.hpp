#pragma once

/**
 * @file Timer.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <etl/list.h>

#include <libnpos/dev/msg/Timer.hpp>
#include <libnpos/nps/Task.hpp>
#include <libnpos/ipc/Port.hpp>
#include <libnpos/ipc/Bus.hpp>
#include <libnpos/dev/hal/SystemTick.hpp>

namespace npos::dev::task
{
    class Timer : public nps::Task, public ipc::Port
    {
    public:
        struct Event
        {
            Tick nextEvent;
            ipc::Oid oid;
            msg::Timer::Mode mode;
        };

        using EventListInt = etl::ilist<Event>;

        //open, timerRegister
        // virtual void initialize() {}
        bool isReady() const override
        {
            for(auto event : eventList)
            {
                if(event.nextEvent <= systemTick.getTick())
                    return true;
            }

            return false;
        }
        
        void process() override
        {
            for(auto& event : eventList)
            {
                if(event.nextEvent <= systemTick.getTick())
                {
                    
                }
            }
        }

    private:
        EventListInt& eventList;
        hal::SystemTick& systemTick;
        ipc::Bus& bus;
    };
}