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
#include <libnpos/dev/Api.hpp>
#include <libnpos/fs/Api.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos;

TEST_GROUP(System)
{
    os::UserApi api;
    mock::Probe probe{api};
    ipc::Service* services[1] = { &probe };
    os::System::Hal hal;
    os::System::Config config;

    // Stops the system once the probe sees the expected response type.
    void stopOn(os::System& system, ipc::Message::Type type, std::function<void(ipc::Message&)> check = nullptr)
    {
        probe.onMessage = [&system, type, check](mock::Probe&, ipc::Message& message)
        {
            if(message.type != type)
                return;

            if(check)
                check(message);

            system.getScheduler().stop();
        };
    }
};

TEST(System, BindsUserApiToUserTask)
{
    os::System system(hal, {}, api);

    CHECK_TRUE(api.isBound());
    CHECK_EQUAL(system.getUserTask().getPortId(), api.getPortId());
}

TEST(System, WithoutFlashThereIsNoFilesystem)
{
    os::System system(hal, {}, api);

    CHECK_FALSE(system.hasFilesystem());
}

TEST(System, FlashAddsFilesystemTask)
{
    emulator::Flash flash;
    emulator::ICache icache;
    hal.flash = &flash;
    hal.icache = &icache;

    os::System system(hal, {}, api);

    CHECK_TRUE(system.hasFilesystem());
}

TEST(System, CliIsOptional)
{
    {
        os::System system(hal, {}, api);
        CHECK_FALSE(system.hasCli());
    }

    config.cli = true;
    os::System system(hal, {}, api, config);

    CHECK_TRUE(system.hasCli());
}

TEST(System, TerminalReceivesLogsOnlyWhenCliIsEnabled)
{
    {
        os::System system(hal, {}, api);
        CHECK_FALSE(system.getUserTask().accepts(ipc::OS_SYSLOG_MESSAGE_TYPE));
    }

    config.cli = true;
    os::System system(hal, {}, api, config);

    CHECK_TRUE(system.getUserTask().accepts(ipc::OS_SYSLOG_MESSAGE_TYPE));
}

TEST(System, TerminalIsTheFirstUserService)
{
    config.cli = true;
    os::System system(hal, services, api, config);

    CHECK_EQUAL(1, probe.getOid().serviceId);
}

TEST(System, UserServicesAreSubscribedInGivenOrder)
{
    os::System system(hal, services, api);

    CHECK_EQUAL(0, probe.getOid().serviceId);
}

TEST(System, UserServicesAreInitializedWhenSchedulerStarts)
{
    emulator::Rtc rtc;
    hal.rtc.hal = &rtc;
    os::System system(hal, services, api);
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
    hal.rtc.hal = &rtc;
    os::System system(hal, services, api);
    ipc::Oid clock = ipc::NULL_OID;
    npos::dev::Timestamp timestamp = 0;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "clock"); };
    probe.onMessage = [&](mock::Probe& self, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN)
        {
            clock = static_cast<dev::msg::Open&>(message).getOpened();
            dev::api::getTime(self, clock);
        }
        else if(message.type == dev::msg::Type::CLOCK)
        {
            timestamp = static_cast<dev::msg::Clock&>(message).getOutputTimestamp();
            system.getScheduler().stop();
        }
    };

    system.start();

    CHECK_EQUAL(1790686224, timestamp);
}

TEST(System, DeviceNamesCanBeChosen)
{
    emulator::Rtc rtc;
    hal.rtc = { &rtc, "rtc7" };
    os::System system(hal, services, api);
    dev::msg::Open::Status status = dev::msg::Open::NOT_FOUND;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "rtc7"); };
    stopOn(system, dev::msg::Type::OPEN, [&](ipc::Message& message)
    {
        status = static_cast<dev::msg::Open&>(message).getStatus();
    });

    system.start();

    CHECK_EQUAL(dev::msg::Open::SUCCESS, status);
}

TEST(System, LedsAreOpenedByTheirName)
{
    emulator::Led led;
    dev::hal::Led* leds[] = { &led };
    hal.leds = leds;
    os::System system(hal, services, api);
    dev::msg::Open::Status status = dev::msg::Open::NOT_FOUND;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::open(self, "leds"); };
    stopOn(system, dev::msg::Type::OPEN, [&](ipc::Message& message)
    {
        status = static_cast<dev::msg::Open&>(message).getStatus();
    });

    system.start();

    CHECK_EQUAL(dev::msg::Open::SUCCESS, status);
}

TEST(System, InterruptDevicesAreServedByTheirOwnTask)
{
    emulator::UartInterrupt uart;
    emulator::Hash hash;
    hal.uart.hal = &uart;
    hal.hash.hal = &hash;
    os::System system(hal, services, api);
    int opened = 0;

    probe.onInitialize = [&](mock::Probe& self)
    {
        dev::api::open(self, "uart0");
        dev::api::open(self, "hash");
    };
    probe.onMessage = [&](mock::Probe&, ipc::Message& message)
    {
        if(message.type == dev::msg::Type::OPEN and static_cast<dev::msg::Open&>(message).getStatus() == dev::msg::Open::SUCCESS)
            opened++;

        if(opened == 2)
            system.getScheduler().stop();
    };

    system.start();

    CHECK_EQUAL(2, opened);
    CHECK_TRUE(uart.initialized);
    CHECK_TRUE(hash.initialized);
}

TEST(System, TimerTaskAnswersUserTimers)
{
    emulator::SystemTick tick;
    hal.systemTick = &tick;
    os::System system(hal, services, api);
    bool fired = false;

    probe.onInitialize = [&](mock::Probe& self) { dev::api::timerRegister(self, dev::msg::Timer::SINGLE_SHOT, 1); };
    stopOn(system, dev::msg::Type::TIMER, [&](ipc::Message&) { fired = true; });

    system.start();

    CHECK_TRUE(fired);
}

TEST(System, FilesystemSavesFilesWrittenByUserServices)
{
    enum class State { OPEN_FOR_WRITE, WRITE, CLOSE, OPEN_FOR_READ, READ };

    emulator::Flash flash;
    emulator::ICache icache;
    hal.flash = &flash;
    hal.icache = &icache;
    os::System system(hal, services, api);
    const uint8_t data[3] = { 'a', 'b', 'c' };
    uint8_t content[8] = {};
    size_t length = 0;
    auto state = State::OPEN_FOR_WRITE;

    probe.onInitialize = [&](mock::Probe& self) { fs::api::file::open(self, "f.txt", fs::Flags::WRITE_ONLY | fs::Flags::CREATE); };
    probe.onMessage = [&](mock::Probe& self, ipc::Message& message)
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
