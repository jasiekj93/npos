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
        npos::dev::message::OpenRequest request(name);
        sender.getApi().send(request);
    }

    void getTime(ipc::Service& sender, ipc::Oid oid)
    {
        npos::dev::message::ClockRequest request(oid, npos::dev::message::ClockRequest::GET_TIME);
        sender.getApi().send(request);
    }

    void setTime(ipc::Service& sender, ipc::Oid oid, Timestamp timestamp)
    {
        npos::dev::message::ClockRequest request(oid, npos::dev::message::ClockRequest::SET_TIME, timestamp);
        sender.getApi().send(request);
    }
}