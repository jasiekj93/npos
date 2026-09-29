#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/message/Clock.hpp>
#include <libnpos/dev/message/Open.hpp>

namespace npos::dev::api
{
    void open(ipc::Service& sender, etl::string_view name)
    {
        npos::dev::message::Open request(name);
        sender.getApi().send(request);
    }

    void getTime(ipc::Service& sender, ipc::Oid oid)
    {
        npos::dev::message::Clock request(oid, npos::dev::message::Clock::GET_TIME);
        sender.getApi().send(request);
    }

    void setTime(ipc::Service& sender, ipc::Oid oid, Timestamp timestamp)
    {
        npos::dev::message::Clock request(oid, npos::dev::message::Clock::SET_TIME, timestamp);
        sender.getApi().send(request);
    }
}