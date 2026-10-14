#pragma once

/**
 * @file WatchdogTask.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/nps/Task.hpp>
#include <libnpos/dev/hal/Watchdog.hpp>

namespace npos::dev::task
{
    class Watchdog: public nps::Task
    {
    public:
        static constexpr Priority PRIORITY = 0;

        explicit Watchdog(hal::Watchdog& watchdog)
            : nps::Task(PRIORITY)
            , watchdog(watchdog)
        {
        }

        void initialize() override 
        {
            if(not watchdog.init())
                return; //TODO handle
        }

        inline bool isReady() const override { return true; }

        void process() override
        {
            watchdog.refresh();
        }

    private:
        hal::Watchdog& watchdog;
    };
}