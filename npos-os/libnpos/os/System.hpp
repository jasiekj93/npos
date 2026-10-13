#pragma once

/**
 * @file System.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/optional.h>
#include <etl/pool.h>
#include <etl/span.h>
#include <etl/string_view.h>
#include <etl/vector.h>

#include <libnpos/nps/Scheduler.hpp>
#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/dev/InterruptServiceTask.hpp>
#include <libnpos/dev/service/LedHandler.hpp>
#include <libnpos/dev/service/RtcClock.hpp>
#include <libnpos/dev/service/UartInterrupt.hpp>
#include <libnpos/dev/service/HashProcessor.hpp>
#include <libnpos/dev/task/Timer.hpp>
#include <libnpos/fs/task/Littlefs.hpp>
#include <libnpos/cli/service/Terminal.hpp>
#include <libnpos/cli/program/Cat.hpp>
#include <libnpos/cli/program/Echo.hpp>
#include <libnpos/cli/program/Touch.hpp>
#include <libnpos/cli/program/Ls.hpp>
#include <libnpos/cli/program/Cd.hpp>
#include <libnpos/cli/program/Pwd.hpp>

#include <libnpos/os/UserApi.hpp>

namespace npos::os
{
    // Builds the whole system from hal interfaces: user, devices, interrupt devices, filesystem and timer tasks.
    // Hal pointers left null mean the device (or the whole task) is not present.
    class System
    {
    public:
        static constexpr size_t MAX_TASKS = 8;
        static constexpr size_t MAX_SERVICES = 32;
        static constexpr size_t QUEUE_SIZE = 32;
        static constexpr size_t POOL_SIZE = 64;
        static constexpr size_t MAX_OPEN = 8;
        static constexpr size_t MAX_LEDS = 8;

        static constexpr ipc::PortId INITIAL_PORT = 0;

        static constexpr nps::Task::Priority USER_PRIORITY = 1;
        static constexpr nps::Task::Priority FILESYSTEM_PRIORITY = 4;
        static constexpr nps::Task::Priority DEVICES_PRIORITY = 5;
        static constexpr nps::Task::Priority INTERRUPT_DEVICES_PRIORITY = 6;
        static constexpr nps::Task::Priority TIMER_PRIORITY = 9;

        template<typename T>
        struct Device
        {
            T* hal = nullptr;
            etl::string_view name;
        };

        struct Hal
        {
            fs::hal::Flash* flash = nullptr;
            fs::hal::ICache* icache = nullptr;

            dev::hal::SystemTick* systemTick = nullptr;
            etl::span<dev::hal::Led* const> leds;
            etl::string_view ledsName = "leds";

            Device<dev::hal::Rtc> rtc = { nullptr, "clock" };
            Device<dev::hal::UartInterrupt> uart = { nullptr, "uart0" };
            Device<dev::hal::Hash> hash = { nullptr, "hash" };
        };

        struct Config
        {
            bool cli = false;
            // Name of the transciever device (e.g. uart) the terminal runs on.
            etl::string_view terminalDevice = "uart0";
        };

        using ServiceList = etl::span<ipc::Service* const>;

        System(const Hal& hal, ServiceList userServices, UserApi& userApi)
            : System(hal, userServices, userApi, Config())
        {
        }

        System(const Hal& hal, ServiceList userServices, UserApi& userApi, const Config& config)
            : scheduler(taskList)
            , bus(portList, messagePool, INITIAL_PORT)
            , devTask(DEVICES_PRIORITY, devServiceList, devMessageQueue, bus)
            , interruptTask(INTERRUPT_DEVICES_PRIORITY, interruptServiceList, interruptMessageQueue, bus)
            , userTask(USER_PRIORITY, userServiceList, userMessageQueue, bus)
        {
            buildDevices(hal);
            buildInterruptDevices(hal);
            buildTimer(hal);
            buildFilesystem(hal);
            buildUser(config, userServices, userApi);
        }

        System(const System&) = delete;
        System& operator=(const System&) = delete;

        void start() { scheduler.start(); }

        inline nps::Scheduler& getScheduler() { return scheduler; }
        inline ipc::SystemBus& getBus() { return bus; }
        inline ipc::ServiceTask& getUserTask() { return userTask; }
        inline bool hasFilesystem() const { return filesystemTask.has_value(); }
        inline bool hasCli() const { return terminal.has_value(); }

    private:
        void addTask(nps::Task& task, ipc::Port& port)
        {
            bus.addPort(port);
            scheduler.addTask(task);
        }

        void buildDevices(const Hal& hal)
        {
            for(auto led : hal.leds)
                leds.push_back(led);

            if(not leds.empty())
            {
                ledHandler.emplace(devTask, leds, hal.ledsName);
                devTask.subscribe(*ledHandler);
            }

            if(hal.rtc.hal)
            {
                rtcClock.emplace(devTask, *hal.rtc.hal, hal.rtc.name);
                devTask.subscribe(*rtcClock);
            }

            if(not devServiceList.empty())
                addTask(devTask, devTask);
        }

        void buildInterruptDevices(const Hal& hal)
        {
            if(hal.uart.hal)
            {
                uartInterrupt.emplace(interruptTask, uartInterruptQueue, *hal.uart.hal, hal.uart.name);
                interruptTask.subscribe(*uartInterrupt);
            }

            if(hal.hash.hal)
            {
                hashProcessor.emplace(interruptTask, hashInterruptQueue, *hal.hash.hal, hal.hash.name);
                interruptTask.subscribe(*hashProcessor);
            }

            if(not interruptServiceList.empty())
                addTask(interruptTask, interruptTask);
        }

        void buildTimer(const Hal& hal)
        {
            if(not hal.systemTick)
                return;

            timerTask.emplace(TIMER_PRIORITY, timerMessageQueue, bus, *hal.systemTick);
            addTask(*timerTask, *timerTask);
        }

        void buildFilesystem(const Hal& hal)
        {
            if(not hal.flash)
                return;

            filesystemTask.emplace(FILESYSTEM_PRIORITY, filesystemMessageQueue, bus, *hal.flash, hal.icache);
            addTask(*filesystemTask, *filesystemTask);
        }

        void buildUser(const Config& config, ServiceList userServices, UserApi& userApi)
        {
            // The terminal must be the first service, ipc routes responses by service id.
            if(config.cli)
            {
                terminal.emplace(userTask, programList, config.terminalDevice);
                userTask.subscribe(*terminal);

                cat.emplace(userTask);
                echo.emplace(userTask);
                touch.emplace(userTask);
                ls.emplace(userTask);
                cd.emplace(userTask);
                pwd.emplace(userTask);

                terminal->addProgram(*cat);
                terminal->addProgram(*echo);
                terminal->addProgram(*touch);
                terminal->addProgram(*ls);
                terminal->addProgram(*cd);
                terminal->addProgram(*pwd);
            }

            for(auto service : userServices)
                userTask.subscribe(*service);

            userApi.bind(userTask);
            addTask(userTask, userTask);
        }

        nps::Scheduler::TaskList<MAX_TASKS> taskList;
        nps::Scheduler scheduler;

        ipc::SystemBus::PortList<MAX_TASKS> portList;
        etl::pool<ipc::Message, POOL_SIZE> messagePool;
        ipc::SystemBus bus;

        // Devices without interrupts
        ipc::ServiceTask::ServiceList<MAX_SERVICES> devServiceList;
        ipc::ServiceTask::MessageQueue<QUEUE_SIZE> devMessageQueue;
        ipc::ServiceTask devTask;
        etl::vector<dev::hal::Led*, MAX_LEDS> leds;
        etl::optional<dev::service::LedHandler> ledHandler;
        etl::optional<dev::service::RtcClock> rtcClock;

        // Devices with interrupts
        dev::InterruptServiceTask::InterruptServiceList<MAX_SERVICES> interruptServiceList;
        dev::InterruptServiceTask::MessageQueue<QUEUE_SIZE> interruptMessageQueue;
        dev::InterruptServiceTask interruptTask;
        dev::InterruptService::InterruptQueue<QUEUE_SIZE> uartInterruptQueue;
        dev::InterruptService::InterruptQueue<QUEUE_SIZE> hashInterruptQueue;
        etl::optional<dev::service::UartInterrupt> uartInterrupt;
        etl::optional<dev::service::HashProcessor> hashProcessor;

        // Timer
        dev::task::Timer::MessageQueue<QUEUE_SIZE> timerMessageQueue;
        etl::optional<dev::task::Timer> timerTask;

        // Filesystem
        fs::task::Littlefs<MAX_OPEN>::MessageQueue<QUEUE_SIZE> filesystemMessageQueue;
        etl::optional<fs::task::Littlefs<MAX_OPEN>> filesystemTask;

        // User space
        ipc::ServiceTask::ServiceList<MAX_SERVICES> userServiceList;
        ipc::ServiceTask::MessageQueue<QUEUE_SIZE> userMessageQueue;
        ipc::ServiceTask userTask;
        cli::service::Terminal::ProgramList<8> programList;
        etl::optional<cli::service::Terminal> terminal;
        etl::optional<cli::program::Cat> cat;
        etl::optional<cli::program::Echo> echo;
        etl::optional<cli::program::Touch> touch;
        etl::optional<cli::program::Ls> ls;
        etl::optional<cli::program::Cd> cd;
        etl::optional<cli::program::Pwd> pwd;
    };
}
