#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 * @brief 
 * @details
 */

#include <etl/optional.h>

#include <libnpos/os/Timestamp.hpp>
#include <libnpos/os/message/RequestResponse.hpp>
#include <libnpos/os/driver/message/Id.hpp>

namespace npos::os::driver::message
{
    class Clock 
    {
    public:
        struct SetTimeRequest : public os::message::Request
        {
            static constexpr auto ID = driver::message::Id::CLOCK_SET_TIME_REQUEST;

            Timestamp timestamp;

            SetTimeRequest(Request::SenderId senderId, Timestamp timestamp) 
                : os::message::Request(ID)
                , senderId(senderId)
                , timestamp(timestamp) {}
        };

        struct SetTimeResponse : public os::message::Response
        {
            static constexpr auto ID = driver::message::Id::CLOCK_SET_TIME_RESPONSE;

            enum Status
            {
                OK,
                RTC_ERROR,
                INVALID_TIMESTAMP
            };

            Status status;
            Timestamp timestamp;

            SetTimeResponse() : os::message::Response(ID) {}
            SetTimeResponse(Status status, Timestamp timestamp = 0) 
                : os::message::Response(ID)
                , status(status)
                , timestamp(timestamp) {}
        };

        struct GetTimeRequest : public os::message::Request
        {
            static constexpr auto ID = driver::message::Id::CLOCK_GET_TIME_REQUEST;

            GetTimeRequest(Request::SenderId senderId) 
                : os::message::Request(ID)
                , senderId(senderId) {}
        };

        struct GetTimeResponse : public os::message::Response
        {
            static constexpr auto ID = driver::message::Id::CLOCK_GET_TIME_RESPONSE;

            etl::optional<Timestamp> timestamp;

            GetTimeResponse() 
                : os::message::Response(ID)
                , timestamp(etl::nullopt) {}

            explicit GetTimeResponse(etl::optional<Timestamp> timestamp) 
                : os::message::Response(ID)
                , timestamp(timestamp) {}
        };
    };
}

