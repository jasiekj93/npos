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
}