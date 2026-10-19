#pragma once

/**
 * @file System.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <algorithm>
#include <utility>

#include <etl/optional.h>
#include <etl/pool.h>
#include <etl/queue.h>
#include <etl/span.h>
#include <etl/string_view.h>
#include <etl/vector.h>

#include <libnpos/nps/Scheduler.hpp>
#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/dev/InterruptServiceTask.hpp>
#include <libnpos/dev/service/BackupStore.hpp>
#include <libnpos/dev/service/HashProcessor.hpp>
#include <libnpos/dev/service/LedHandler.hpp>
#include <libnpos/dev/service/RtcClock.hpp>
#include <libnpos/dev/service/UartInterrupt.hpp>
#include <libnpos/dev/service/UartPolling.hpp>
#include <libnpos/dev/task/Idle.hpp>
#include <libnpos/dev/task/Timer.hpp>
#include <libnpos/dev/task/Watchdog.hpp>
#include <libnpos/fs/task/Littlefs.hpp>
#include <libnpos/cli/task/Terminal.hpp>
#include <libnpos/cli/utils/LogStream.hpp>
#include <libnpos/cli/program/Cat.hpp>
#include <libnpos/cli/program/Echo.hpp>
#include <libnpos/cli/program/Touch.hpp>
#include <libnpos/cli/program/Ls.hpp>
#include <libnpos/cli/program/Cd.hpp>
#include <libnpos/cli/program/Pwd.hpp>
#include <libnpos/cli/program/Mkdir.hpp>
#include <libnpos/cli/program/Random.hpp>
#include <libnpos/cli/program/Date.hpp>

#include <libnpos/os/UserApi.hpp>

namespace npos::os
{
    template<typename T>
    struct Device
    {
        T& hal;
        etl::string_view name;
    };

    template<typename T>
    struct InterruptDevice
    {
        T& hal;
        etl::string_view name;
        dev::InterruptService::InterruptQueueInt& queue;
    };

    struct LedBank
    {
        dev::service::LedHandler::Leds& leds;
        etl::string_view name;
    };

    // Hardware given to the system. Null pointers and empty spans mean the device is not present.
    struct Hal
    {
        explicit Hal(dev::hal::SystemTick& systemTick) : systemTick(systemTick) {}

        dev::hal::SystemTick& systemTick;

        fs::hal::Flash* flash = nullptr;
        fs::hal::ICache* icache = nullptr;

        // Without power there is no idle task, without watchdog there is no watchdog task.
        dev::hal::Power* power = nullptr;
        dev::hal::Watchdog* watchdog = nullptr;

        etl::span<const LedBank> leds;
        etl::span<const Device<dev::hal::Rtc>> rtcs;
        etl::span<const Device<dev::hal::BackupRegisters>> backups;
        etl::span<const Device<dev::hal::UartPolling>> pollingUarts;
        etl::span<const InterruptDevice<dev::hal::UartInterrupt>> uarts;
        etl::span<const InterruptDevice<dev::hal::Hash>> hashes;
    };

    struct Priorities
    {
        nps::Task::Priority user = 1;
        nps::Task::Priority terminal = 2;
        nps::Task::Priority filesystem = 4;
        nps::Task::Priority devices = 5;
        nps::Task::Priority interruptDevices = 6;
        nps::Task::Priority timer = 9;
    };

    struct Config
    {
        bool cli = false;
        // Name of the transciever device (e.g. uart) the terminal runs on.
        etl::string_view terminalDevice = "uart0";
        // Name of the clock device (rtc) the date program uses.
        etl::string_view clockDevice = "clock";
        Priorities priorities;
    };

    // Size of one slot in the pool of device services.
    struct ServiceSlot
    {
        static constexpr size_t SIZE = std::max({ sizeof(dev::service::BackupStore), sizeof(dev::service::HashProcessor),
            sizeof(dev::service::LedHandler), sizeof(dev::service::RtcClock), sizeof(dev::service::UartInterrupt),
            sizeof(dev::service::UartPolling) });

        static constexpr size_t ALIGNMENT = std::max({ alignof(dev::service::BackupStore), alignof(dev::service::HashProcessor),
            alignof(dev::service::LedHandler), alignof(dev::service::RtcClock), alignof(dev::service::UartInterrupt),
            alignof(dev::service::UartPolling) });
    };

    // Memory the system works in; it must outlive the system. See Storage for a ready made set.
    struct Memory
    {
        nps::Scheduler::TaskListInt& tasks;
        ipc::SystemBus::PortListInt& ports;
        ipc::SystemBus::PoolInt& messages;
        // Slots of ServiceSlot::SIZE bytes, one per device service.
        etl::ipool& deviceServicePool;

        ipc::ServiceTask::ServiceListInt& deviceServices;
        ipc::ServiceTask::RequestQueueInt& deviceRequests;
        ipc::ServiceTask::ResponseQueueInt& deviceResponses;
        dev::InterruptServiceTask::InterruptServiceListInt& interruptServices;
        ipc::ServiceTask::RequestQueueInt& interruptRequests;
        ipc::ServiceTask::ResponseQueueInt& interruptResponses;
        ipc::ServiceTask::ServiceListInt& userServices;
        ipc::ServiceTask::RequestQueueInt& userRequests;
        ipc::ServiceTask::ResponseQueueInt& userResponses;
        dev::task::Timer::MessageQueueInt& timerQueue;
        ipc::ServiceTask::RequestQueueInt& filesystemQueue;
        cli::task::Terminal::MessageQueueInt& terminalQueue;
        // Slots of ipc::log::Stream::MAX_MESSAGE_LENGTH bytes, the terminal keeps the text of queued logs in them.
        cli::task::Terminal::PayloadPoolInt& logPayloads;
        cli::task::Terminal::ProgramListInt& programs;
    };

    template<size_t Tasks = 8, size_t Services = 32, size_t Queue = 32, size_t Messages = 64, size_t DeviceServices = 16, size_t Programs = 16, size_t Logs = 8>
    class Storage
    {
    public:
        Memory memory()
        {
            return Memory{ taskList, portList, messagePool, deviceServicePool, deviceServiceList, deviceRequests, deviceResponses,
                interruptServiceList, interruptRequests, interruptResponses, userServiceList, userRequests, userResponses,
                timerQueue, filesystemQueue, terminalQueue, logPayloadPool, programList };
        }

    private:
        nps::Scheduler::TaskList<Tasks> taskList;
        ipc::SystemBus::PortList<Tasks> portList;
        etl::pool<ipc::Message, Messages> messagePool;
        etl::generic_pool<ServiceSlot::SIZE, ServiceSlot::ALIGNMENT, DeviceServices> deviceServicePool;
        ipc::ServiceTask::ServiceList<Services> deviceServiceList;
        ipc::ServiceTask::RequestQueue<Queue> deviceRequests;
        ipc::ServiceTask::ResponseQueue<Queue> deviceResponses;
        dev::InterruptServiceTask::InterruptServiceList<Services> interruptServiceList;
        ipc::ServiceTask::RequestQueue<Queue> interruptRequests;
        ipc::ServiceTask::ResponseQueue<Queue> interruptResponses;
        ipc::ServiceTask::ServiceList<Services> userServiceList;
        ipc::ServiceTask::RequestQueue<Queue> userRequests;
        ipc::ServiceTask::ResponseQueue<Queue> userResponses;
        dev::task::Timer::MessageQueue<Queue> timerQueue;
        ipc::ServiceTask::RequestQueue<Queue> filesystemQueue;
        cli::task::Terminal::MessageQueue<Queue> terminalQueue;
        etl::generic_pool<cli::utils::LogStream::MAX_MESSAGE_LENGTH, 1, Logs> logPayloadPool;
        cli::task::Terminal::ProgramList<Programs> programList;
    };

    // Builds the whole system from hal interfaces: user, devices, interrupt devices, filesystem, timer, idle and watchdog tasks.
    template<size_t MaxOpen = 8>
    class BasicSystem
    {
    public:
        static constexpr ipc::PortId INITIAL_PORT = 0;

        using ServiceList = etl::span<ipc::Service* const>;

        BasicSystem(const Hal& hal, Memory memory, ServiceList userServices, UserApi& userApi)
            : BasicSystem(hal, memory, userServices, userApi, Config())
        {
        }

        BasicSystem(const Hal& hal, Memory memory, ServiceList userServices, UserApi& userApi, const Config& config)
            : memory(memory)
            , scheduler(memory.tasks)
            , bus(memory.ports, memory.messages, INITIAL_PORT)
            , devTask(config.priorities.devices, memory.deviceServices, memory.deviceRequests, memory.deviceResponses, bus)
            , interruptTask(config.priorities.interruptDevices, memory.interruptServices, memory.interruptRequests,
                memory.interruptResponses, bus)
            , userTask(config.priorities.user, memory.userServices, memory.userRequests, memory.userResponses, bus)
        {
            buildDevices(hal);
            buildInterruptDevices(hal);
            buildTimer(hal, config);
            buildFilesystem(hal, config);
            buildSupervision(hal);
            buildUser(config, userServices, userApi);
        }

        BasicSystem(const BasicSystem&) = delete;
        BasicSystem& operator=(const BasicSystem&) = delete;

        void start() { scheduler.start(); }

        inline nps::Scheduler& getScheduler() { return scheduler; }
        inline ipc::SystemBus& getBus() { return bus; }
        inline ipc::ServiceTask& getUserTask() { return userTask; }
        inline bool hasFilesystem() const { return filesystemTask.has_value(); }
        inline bool hasCli() const { return terminal.has_value(); }
        inline cli::task::Terminal* getTerminal() { return terminal ? &*terminal : nullptr; }
        inline bool hasIdleTask() const { return idleTask.has_value(); }
        inline bool hasWatchdogTask() const { return watchdogTask.has_value(); }

    private:
        struct Programs
        {
            explicit Programs(ipc::Service::Api& api, etl::string_view clockDevice)
                : cat(api), echo(api), touch(api), ls(api), cd(api), pwd(api), mkdir(api), random(api), date(api, clockDevice)
            {
            }

            cli::program::Cat cat;
            cli::program::Echo echo;
            cli::program::Touch touch;
            cli::program::Ls ls;
            cli::program::Cd cd;
            cli::program::Pwd pwd;
            cli::program::Mkdir mkdir;
            cli::program::Random random;
            cli::program::Date date;
        };

        void addTask(nps::Task& task, ipc::Port& port)
        {
            bus.addPort(port);
            scheduler.addTask(task);
        }

        // Services that do not fit in the pool are skipped.
        template<typename T, typename... Args>
        T* make(Args&&... args)
        {
            return memory.deviceServicePool.template create<T>(std::forward<Args>(args)...);
        }

        void buildDevices(const Hal& hal)
        {
            for(auto& bank : hal.leds)
                if(auto service = make<dev::service::LedHandler>(devTask, bank.leds, bank.name))
                    devTask.subscribe(*service);

            for(auto& rtc : hal.rtcs)
                if(auto service = make<dev::service::RtcClock>(devTask, rtc.hal, rtc.name))
                    devTask.subscribe(*service);

            for(auto& backup : hal.backups)
                if(auto service = make<dev::service::BackupStore>(devTask, backup.hal, backup.name))
                    devTask.subscribe(*service);

            for(auto& uart : hal.pollingUarts)
                if(auto service = make<dev::service::UartPolling>(devTask, uart.hal, uart.name))
                    devTask.subscribe(*service);

            if(not memory.deviceServices.empty())
                addTask(devTask, devTask);
        }

        void buildInterruptDevices(const Hal& hal)
        {
            for(auto& uart : hal.uarts)
                if(auto service = make<dev::service::UartInterrupt>(interruptTask, uart.queue, uart.hal, uart.name))
                    interruptTask.subscribe(*service);

            for(auto& hash : hal.hashes)
                if(auto service = make<dev::service::HashProcessor>(interruptTask, hash.queue, hash.hal, hash.name))
                    interruptTask.subscribe(*service);

            if(not memory.interruptServices.empty())
                addTask(interruptTask, interruptTask);
        }

        void buildTimer(const Hal& hal, const Config& config)
        {
            timerTask.emplace(config.priorities.timer, memory.timerQueue, bus, hal.systemTick);
            addTask(*timerTask, *timerTask);
        }

        void buildFilesystem(const Hal& hal, const Config& config)
        {
            if(not hal.flash)
                return;

            filesystemTask.emplace(config.priorities.filesystem, memory.filesystemQueue, bus, *hal.flash, hal.icache);
            addTask(*filesystemTask, *filesystemTask);
        }

        // Idle and watchdog tasks have a fixed priority and are run by the scheduler outside of its task list.
        void buildSupervision(const Hal& hal)
        {
            if(hal.power)
            {
                idleTask.emplace(*hal.power);
                scheduler.setIdleTask(*idleTask);
            }

            if(hal.watchdog)
            {
                watchdogTask.emplace(*hal.watchdog);
                scheduler.setWatchdogTask(*watchdogTask);
            }
        }

        void buildUser(const Config& config, ServiceList userServices, UserApi& userApi)
        {
            if(config.cli)
            {
                terminal.emplace(config.priorities.terminal, memory.terminalQueue, memory.logPayloads, memory.programs,
                    bus, config.terminalDevice);
                programs.emplace(*terminal, config.clockDevice);

                terminal->addProgram(programs->cat);
                terminal->addProgram(programs->echo);
                terminal->addProgram(programs->touch);
                terminal->addProgram(programs->ls);
                terminal->addProgram(programs->cd);
                terminal->addProgram(programs->pwd);
                terminal->addProgram(programs->mkdir);
                terminal->addProgram(programs->random);
                terminal->addProgram(programs->date);
                addTask(*terminal, *terminal);
            }

            for(auto service : userServices)
                userTask.subscribe(*service);

            userApi.bind(userTask);
            addTask(userTask, userTask);
        }

        Memory memory;
        nps::Scheduler scheduler;
        ipc::SystemBus bus;

        ipc::ServiceTask devTask;
        dev::InterruptServiceTask interruptTask;
        ipc::ServiceTask userTask;

        etl::optional<dev::task::Timer> timerTask;
        etl::optional<fs::task::Littlefs<MaxOpen>> filesystemTask;
        etl::optional<dev::task::Idle> idleTask;
        etl::optional<dev::task::Watchdog> watchdogTask;
        etl::optional<cli::task::Terminal> terminal;
        etl::optional<Programs> programs;
    };

    using System = BasicSystem<>;
}
