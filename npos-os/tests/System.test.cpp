/**
 * @file System.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/Probe.hpp"

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include <libnpos/emulator/Led.hpp>
#include <libnpos/emulator/Rtc.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include <libnpos/emulator/Hash.hpp>
#include <libnpos/emulator/UartInterrupt.hpp>
#include <libnpos/emulator/UartPolling.hpp>
#include <libnpos/emulator/BackupRegisters.hpp>
#include <libnpos/emulator/Power.hpp>
#include <libnpos/emulator/Watchdog.hpp>
#include <libnpos/dev/Api.hpp>
#include <libnpos/dev/service/Heartbeat.hpp>
#include <libnpos/fs/Api.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos;

TEST_GROUP(System)
{
    emulator::SystemTick tick;
    os::Storage<> storage;
    os::UserApi api;
    mock::Probe probe{api};
    ipc::Service* services[1] = { &probe };
    os::Hal hal{tick};
    os::Config config;

    // Stops the system once the probe sees the expected response type.
    void stopOn(os::System& system, ipc::Message::Type type, std::function<void(ipc::Message&)> check = nullptr)
    {
        probe.onResponse = [&system, type, check](mock::Probe&, ipc::Message& message)
        {
            if(message.type != type)
                return;

            if(check)
                check(message);

            system.getScheduler().stop();
        };
    }

    dev::msg::Open::Status openStatus(ipc::Message& message)
    {
        return static_cast<dev::msg::Open&>(message).getStatus();
    }
};

TEST(System, BindsUserApiToUserTask)
{
    os::System system(hal, storage.memory(), {}, api);

    CHECK_TRUE(api.isBound());
    CHECK_EQUAL(system.getUserTask().getPortId(), api.getPortId());
}

TEST(System, WithoutFlashThereIsNoFilesystem)
{
    os::System system(hal, storage.memory(), {}, api);

    CHECK_FALSE(system.hasFilesystem());
}

TEST(System, FlashAddsFilesystemTask)
{
    emulator::Flash flash;
    emulator::ICache icache;
    hal.flash = &flash;
    hal.icache = &icache;

    os::System system(hal, storage.memory(), {}, api);

    CHECK_TRUE(system.hasFilesystem());
}

TEST(System, PowerAddsIdleTaskAndWatchdogAddsWatchdogTask)
{
    emulator::Power power;
    emulator::Watchdog watchdog;

    {
        os::System system(hal, storage.memory(), {}, api);
        CHECK_FALSE(system.hasIdleTask());
        CHECK_FALSE(system.hasWatchdogTask());
    }

    os::Storage<> other;
    hal.power = &power;
    hal.watchdog = &watchdog;
    os::System system(hal, other.memory(), {}, api);

    CHECK_TRUE(system.hasIdleTask());
    CHECK_TRUE(system.hasWatchdogTask());
}

TEST(System, WatchdogIsInitializedAndRefreshedByScheduler)
{
    emulator::Watchdog watchdog;
    emulator::Rtc rtc;
    os::Device<dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };
    hal.watchdog = &watchdog;
    hal.rtcs = rtcs;
    os::System system(hal, storage.memory(), services, api);

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "clock"); };
    stopOn(system, dev::msg::Type::OPEN);
    system.start();

    CHECK_TRUE(watchdog.initialized);
    CHECK_TRUE(watchdog.refreshCount > 0);
}

TEST(System, PrioritiesHaveDefaults)
{
    os::System system(hal, storage.memory(), {}, api);

    CHECK_EQUAL(1, system.getUserTask().getPriority());
}

TEST(System, PrioritiesCanBeConfigured)
{
    config.priorities.user = 3;
    os::System system(hal, storage.memory(), {}, api, config);

    CHECK_EQUAL(3, system.getUserTask().getPriority());
}

TEST(System, CliIsOptional)
{
    {
        os::System system(hal, storage.memory(), {}, api);
        CHECK_FALSE(system.hasCli());
    }

    os::Storage<> other;
    config.cli = true;
    os::System system(hal, other.memory(), {}, api, config);

    CHECK_TRUE(system.hasCli());
}

TEST(System, TerminalIsATaskOfItsOwn)
{
    {
        os::System system(hal, storage.memory(), {}, api);
        CHECK_TRUE(system.getTerminal() == nullptr);
    }

    os::Storage<> other;
    config.cli = true;
    os::System system(hal, other.memory(), {}, api, config);

    CHECK_TRUE(system.getTerminal() != nullptr);
    CHECK_TRUE(system.getTerminal()->accepts(cli::msg::Type::SYSLOG));
    CHECK_FALSE(system.getUserTask().accepts(cli::msg::Type::SYSLOG));
}

TEST(System, TerminalPriorityIsConfigurable)
{
    config.cli = true;
    config.priorities.terminal = 7;
    os::System system(hal, storage.memory(), {}, api, config);

    CHECK_EQUAL(7, system.getTerminal()->getPriority());
}

TEST(System, CliDoesNotChangeUserServiceIds)
{
    config.cli = true;
    os::System system(hal, storage.memory(), services, api, config);

    CHECK_EQUAL(0, probe.getOid().serviceId);
}

TEST(System, UserServicesAreSubscribedInGivenOrder)
{
    os::System system(hal, storage.memory(), services, api);

    CHECK_EQUAL(0, probe.getOid().serviceId);
}

TEST(System, UserServicesAreInitializedWhenSchedulerStarts)
{
    emulator::Rtc rtc;
    os::Device<dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };
    hal.rtcs = rtcs;
    os::System system(hal, storage.memory(), services, api);
    int initialized = 0;

    // The scheduler only honours stop() once it is running, so wait for a response.
    probe.onInitialize = [&](mock::Probe& self)
    {
        initialized++;
        dev::api::open(self, "clock");
    };
    stopOn(system, dev::msg::Type::OPEN);

    system.start();

    CHECK_EQUAL(1, initialized);
}

TEST(System, UserServiceCanReadTheClock)
{
    emulator::Rtc rtc;
    os::Device<dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };
    hal.rtcs = rtcs;
    os::System system(hal, storage.memory(), services, api);
    npos::dev::Timestamp timestamp = 0;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "clock"); };
    probe.onResponse = [&](mock::Probe& self, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN)
            dev::api::getTime(self, static_cast<dev::msg::Open&>(message).getOpened());
        else if(message.type == dev::msg::Type::CLOCK)
        {
            timestamp = static_cast<dev::msg::Clock&>(message).getOutputTimestamp();
            system.getScheduler().stop();
        }
    };

    system.start();

    CHECK_EQUAL(1790686224, timestamp);
}

TEST(System, SeveralDevicesOfOneKindAreServedByName)
{
    emulator::Rtc first, second;
    os::Device<dev::hal::Rtc> rtcs[] = { { first, "rtc1" }, { second, "rtc2" } };
    hal.rtcs = rtcs;
    os::System system(hal, storage.memory(), services, api);
    int opened = 0;

    probe.onInitialize = [&](mock::Probe& self)
    {
        dev::api::open(self, "rtc1");
        dev::api::open(self, "rtc2");
    };
    probe.onResponse = [&](mock::Probe&, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN and openStatus(message) == dev::msg::Open::SUCCESS)
            opened++;

        if(opened == 2)
            system.getScheduler().stop();
    };

    system.start();

    CHECK_EQUAL(2, opened);
}

TEST(System, LedsAreOpenedByTheirName)
{
    emulator::Led led;
    etl::vector<dev::hal::Led*, 1> leds { &led };
    os::LedBank banks[] = { { leds, "leds" } };
    hal.leds = banks;
    os::System system(hal, storage.memory(), services, api);
    dev::msg::Open::Status status = dev::msg::Open::NOT_FOUND;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "leds"); };
    stopOn(system, dev::msg::Type::OPEN, [&](ipc::Message& message) { status = openStatus(message); });

    system.start();

    CHECK_EQUAL(dev::msg::Open::SUCCESS, status);
}

TEST(System, SeveralInterruptUartsAndHashesAreServed)
{
    emulator::UartInterrupt uart0, uart1;
    emulator::Hash hash;
    dev::InterruptService::InterruptQueue<8> queue0, queue1, hashQueue;
    os::InterruptDevice<dev::hal::UartInterrupt> uarts[] = { { uart0, "uart0", queue0 }, { uart1, "uart1", queue1 } };
    os::InterruptDevice<dev::hal::Hash> hashes[] = { { hash, "hash", hashQueue } };
    hal.uarts = uarts;
    hal.hashes = hashes;
    os::System system(hal, storage.memory(), services, api);
    int opened = 0;

    probe.onInitialize = [&](mock::Probe& self)
    {
        dev::api::open(self, "uart0");
        dev::api::open(self, "uart1");
        dev::api::open(self, "hash");
    };
    probe.onResponse = [&](mock::Probe&, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN and openStatus(message) == dev::msg::Open::SUCCESS)
            opened++;

        if(opened == 3)
            system.getScheduler().stop();
    };

    system.start();

    CHECK_EQUAL(3, opened);
    CHECK_TRUE(uart0.initialized);
    CHECK_TRUE(uart1.initialized);
    CHECK_TRUE(hash.initialized);
}

TEST(System, PollingUartsTransmitSynchronously)
{
    emulator::UartPolling uart;
    os::Device<dev::hal::UartPolling> uarts[] = { { uart, "ttyS0" } };
    hal.pollingUarts = uarts;
    os::System system(hal, storage.memory(), services, api);
    const uint8_t data[3] = { 1, 2, 3 };
    dev::msg::Transciever::Status status = dev::msg::Transciever::DEVICE_ERROR;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "ttyS0"); };
    probe.onResponse = [&](mock::Probe& self, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN)
            dev::api::transcieverWrite(self, static_cast<dev::msg::Open&>(message).getOpened(), etl::span<const uint8_t>(data, sizeof(data)));
        else if(message.type == dev::msg::Type::TRANSCIEVER)
        {
            status = static_cast<dev::msg::Transciever&>(message).getStatus();
            system.getScheduler().stop();
        }
    };

    system.start();

    CHECK_EQUAL(dev::msg::Transciever::SUCCESS, status);
    CHECK_EQUAL(3, uart.transmitted);
}

TEST(System, BackupRegistersAreWrittenAndRead)
{
    emulator::BackupRegisters registers;
    os::Device<dev::hal::BackupRegisters> backups[] = { { registers, "backup" } };
    hal.backups = backups;
    os::System system(hal, storage.memory(), services, api);
    dev::msg::BackupRegisters::Value written = 77;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "backup"); };
    probe.onResponse = [&](mock::Probe& self, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN)
            dev::api::backupWrite(self, static_cast<dev::msg::Open&>(message).getOpened(), 4, written);
        else if(message.type == dev::msg::Type::BACKUP_REGISTERS)
            system.getScheduler().stop();
    };

    system.start();

    CHECK_TRUE(registers.accessEnabled);
    CHECK_EQUAL(77, registers.registers[4]);
}

TEST(System, DeviceServicesBeyondPoolSizeAreSkipped)
{
    os::Storage<8, 32, 32, 64, 1> small;
    emulator::Rtc first, second;
    os::Device<dev::hal::Rtc> rtcs[] = { { first, "a" }, { second, "b" } };
    hal.rtcs = rtcs;
    os::System system(hal, small.memory(), services, api);
    dev::msg::Open::Status status = dev::msg::Open::NOT_FOUND;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "a"); };
    stopOn(system, dev::msg::Type::OPEN, [&](ipc::Message& message) { status = openStatus(message); });

    system.start();

    CHECK_EQUAL(dev::msg::Open::SUCCESS, status);
}

TEST(System, TimerTaskAnswersUserTimers)
{
    os::System system(hal, storage.memory(), services, api);
    bool fired = false;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::timerRegister(self, dev::msg::Timer::SINGLE_SHOT, 1); };
    stopOn(system, dev::msg::Type::TIMER, [&](ipc::Message&) { fired = true; });

    system.start();

    CHECK_TRUE(fired);
}

TEST(System, HeartbeatTogglesItsLedOnEveryTimerEvent)
{
    struct CountingLed : public emulator::Led
    {
        void toggle() override
        {
            toggles++;

            if(onToggle)
                onToggle();
        }

        int toggles = 0;
        std::function<void()> onToggle;
    };

    CountingLed led;
    etl::vector<dev::hal::Led*, 1> leds { &led };
    os::LedBank banks[] = { { leds, "leds" } };
    hal.leds = banks;
    dev::service::Heartbeat heartbeat{api, "leds", 0, 1};
    ipc::Service* heartbeatServices[1] = { &heartbeat };
    os::System system(hal, storage.memory(), heartbeatServices, api);
    led.onToggle = [&]() { if(led.toggles == 2) system.getScheduler().stop(); };

    system.start();

    CHECK_EQUAL(2, led.toggles);
}

TEST(System, RequestToUnknownPortIsAnsweredWithPortNotFound)
{
    os::System system(hal, storage.memory(), services, api);
    ipc::Message::Status status = 0;

    probe.onInitialize = [&](mock::Probe& self)
    {
        ipc::Message request;
        request.type = ipc::USER_MESSAGE_TYPE;
        request.recipient = { 200, ipc::NULL_SERVICE };
        self.getApi().send(self, request);
    };
    stopOn(system, ipc::USER_MESSAGE_TYPE, [&](ipc::Message& message) { status = message.output.status; });

    system.start();

    CHECK_EQUAL(ipc::Message::STATUS_PORT_NOT_FOUND, status);
}

TEST(System, RequestTheAddressedPortDoesNotAcceptIsAnsweredWithNotSupported)
{
    emulator::Rtc rtc;
    os::Device<dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };
    hal.rtcs = rtcs;
    os::System system(hal, storage.memory(), services, api);
    ipc::Message::Status status = 0;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "clock"); };
    probe.onResponse = [&](mock::Probe& self, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN)
        {
            ipc::Message request;
            request.type = ipc::USER_MESSAGE_TYPE;
            request.recipient = static_cast<dev::msg::Open&>(message).getOpened();
            self.getApi().send(self, request);
        }
        else if(message.type == ipc::USER_MESSAGE_TYPE)
        {
            status = message.output.status;
            system.getScheduler().stop();
        }
    };

    system.start();

    CHECK_EQUAL(ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, status);
}

TEST(System, FilesystemSavesFilesWrittenByUserServices)
{
    enum class State { OPEN_FOR_WRITE, WRITE, CLOSE, OPEN_FOR_READ, READ };

    emulator::Flash flash;
    emulator::ICache icache;
    hal.flash = &flash;
    hal.icache = &icache;
    os::System system(hal, storage.memory(), services, api);
    const uint8_t data[3] = { 'a', 'b', 'c' };
    uint8_t content[8] = {};
    size_t length = 0;
    auto state = State::OPEN_FOR_WRITE;

    probe.onInitialize = [&](mock::Probe& self) { fs::api::file::open(self, "f.txt", fs::Flags::WRITE_ONLY | fs::Flags::CREATE); };
    probe.onResponse = [&](mock::Probe& self, ipc::Message& message)
    {
        if(message.type == fs::msg::Type::FILE_OPEN and state == State::OPEN_FOR_WRITE)
        {
            state = State::WRITE;
            fs::api::file::write(self, static_cast<fs::msg::file::Open&>(message).getOpened(), etl::span<const uint8_t>(data, sizeof(data)));
        }
        else if(message.type == fs::msg::Type::FILE_WRITE and state == State::WRITE)
        {
            state = State::CLOSE;
            fs::api::file::close(self, message.recipient);
        }
        else if(message.type == fs::msg::Type::FILE_CLOSE and state == State::CLOSE)
        {
            state = State::OPEN_FOR_READ;
            fs::api::file::open(self, "f.txt", fs::Flags::READ_ONLY);
        }
        else if(message.type == fs::msg::Type::FILE_OPEN and state == State::OPEN_FOR_READ)
        {
            state = State::READ;
            fs::api::file::read(self, static_cast<fs::msg::file::Open&>(message).getOpened(), sizeof(content), etl::span<uint8_t>(content, sizeof(content)));
        }
        else if(message.type == fs::msg::Type::FILE_READ and state == State::READ)
        {
            length = static_cast<fs::msg::file::Read&>(message).getOutputLength();
            system.getScheduler().stop();
        }
    };

    system.start();

    CHECK_EQUAL(3, length);
    MEMCMP_EQUAL("abc", content, 3);
}
