#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/Clock.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Led.hpp>
#include <libnpos/dev/msg/Hash.hpp>
#include <libnpos/dev/msg/BackupRegisters.hpp>
#include <libnpos/dev/msg/Transciever.hpp>
#include <libnpos/dev/msg/Timer.hpp>

namespace npos::dev::api
{
    void open(ipc::Service& sender, etl::string_view name)
    {
        npos::dev::msg::Open request(name);
        sender.getApi().send(request);
    }

    void getTime(ipc::Service& sender, ipc::Oid oid)
    {
        npos::dev::msg::Clock request(oid, npos::dev::msg::Clock::GET_TIME);
        sender.getApi().send(request);
    }

    void setTime(ipc::Service& sender, ipc::Oid oid, Timestamp timestamp)
    {
        npos::dev::msg::Clock request(oid, npos::dev::msg::Clock::SET_TIME, timestamp);
        sender.getApi().send(request);
    }

    void ledOn(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::Led::Id ledId)
    {
        npos::dev::msg::Led request(oid, ledId, npos::dev::msg::Led::ON);
        sender.getApi().send(request);
    }

    void ledOff(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::Led::Id ledId)
    {
        npos::dev::msg::Led request(oid, ledId, npos::dev::msg::Led::OFF);
        sender.getApi().send(request);
    }

    void ledToggle(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::Led::Id ledId)
    {
        npos::dev::msg::Led request(oid, ledId, npos::dev::msg::Led::TOGGLE);
        sender.getApi().send(request);
    }

    void hashAccumulate(ipc::Service& sender, ipc::Oid oid, etl::span<const uint8_t> input)
    {
        npos::dev::msg::Hash request(oid, npos::dev::msg::Hash::ACCUMULATE, input);
        sender.getApi().send(request);
    }

    void hashAccumulateEnd(ipc::Service& sender, ipc::Oid oid, etl::span<const uint8_t> input, etl::span<uint8_t> output)
    {
        npos::dev::msg::Hash request(oid, npos::dev::msg::Hash::ACCUMULATE_END, input, output);
        sender.getApi().send(request);
    }

    void backupRead(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::BackupRegisters::Register reg, npos::dev::msg::BackupRegisters::Value& output)
    {
        npos::dev::msg::BackupRegisters request(oid, npos::dev::msg::BackupRegisters::READ, reg, &output);
        sender.getApi().send(request);
    }

    void backupWrite(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::BackupRegisters::Register reg, npos::dev::msg::BackupRegisters::Value& input)
    {
        npos::dev::msg::BackupRegisters request(oid, npos::dev::msg::BackupRegisters::WRITE, reg, &input);
        sender.getApi().send(request);
    }

    void transcieverRead(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::Transciever::Length maxLength, etl::span<uint8_t> output)
    {
        npos::dev::msg::Transciever request(oid, npos::dev::msg::Transciever::READ, maxLength, output);
        sender.getApi().send(request);
    }

    void transcieverWrite(ipc::Service& sender, ipc::Oid oid, etl::span<const uint8_t> input)
    {
        npos::dev::msg::Transciever request(oid, npos::dev::msg::Transciever::WRITE, input.size(), reinterpret_cast<etl::span<uint8_t>&>(input));
        sender.getApi().send(request);
    }

    void timerRegister(ipc::Service& sender, ipc::Oid oid, npos::dev::msg::Timer::Mode mode, Tick interval)
    {
        npos::dev::msg::Timer request(oid, sender.getOid(), mode, interval);
        sender.getApi().send(request);
    }
}