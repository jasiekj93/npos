#pragma once

/**
 * @file IdleTask.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/nps/Task.hpp>
#include <libnpos/dev/hal/Power.hpp>

namespace npos::dev::task
{
    class Idle: public nps::Task
    {
    public:
        static constexpr Priority PRIORITY = 0;

        explicit Idle(hal::Power& power)
            : nps::Task(PRIORITY)
            , power(power)
        {
        }

        inline bool isReady() const override { return true; }

        void process() override
        {
            power.enterSleepMode();
        }

    private:
        hal::Power& power;
    };
}