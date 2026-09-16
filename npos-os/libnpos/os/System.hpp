#pragma once

/**
 * @file System.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/nps/Scheduler.hpp>
#include <libnpos/os/message/Bus.hpp>
#include <libnpos/os/Process.hpp>
#include <libnpos/os/driver/hal/Rtc.hpp>

namespace npos::os
{
    class System
    {
    public:
        struct DeviceList
        {
            driver::hal::Rtc* rtc;
        };

        using TaskStorage = nps::Scheduler::TaskList;

        System(TaskStorage& taskStorage, DeviceList& devices); 

        bool addProcess(os::Process& process);

        void start();

    private:
        nps::Scheduler scheduler;
        DeviceList& devices;
        message::Bus systemBus;
    };
}