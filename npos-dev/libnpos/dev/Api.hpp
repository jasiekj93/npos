#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/Clock.hpp>
#include <libnpos/dev/Open.hpp>

namespace npos::dev::api
{
    void open(ipc::Service& sender, etl::string_view name)
    {
        OpenRequest request(name);
        sender.getApi().send(request);
    }

    void getTime(ipc::Service& sender, ipc::Oid oid)
    {
        ClockRequest request(oid, ClockRequest::GET_TIME);
        sender.getApi().send(request);
    }

    void setTime(ipc::Service& sender, ipc::Oid oid, Timestamp timestamp)
    {
        ClockRequest request(oid, ClockRequest::SET_TIME, timestamp);
        sender.getApi().send(request);
    }
}