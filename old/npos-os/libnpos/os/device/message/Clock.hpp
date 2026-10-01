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
#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    class Clock 
    {
    public:
        struct SetTimeRequest : public os::Message
        {
            static constexpr auto ID = device::message::Id::CLOCK_SET_TIME_REQUEST;

            Timestamp timestamp;

            SetTimeRequest() : os::Message(ID) {}
        };

        struct SetTimeResponse : public os::Message
        {
            static constexpr auto ID = device::message::Id::CLOCK_SET_TIME_RESPONSE;

            enum Status
            {
                OK,
                RTC_ERROR,
                INVALID_TIMESTAMP
            };

            Status status;
            Timestamp timestamp;

            SetTimeResponse() : os::Message(ID) {}
        };

        struct GetTimeRequest : public os::Message
        {
            static constexpr auto ID = device::message::Id::CLOCK_GET_TIME_REQUEST;

            GetTimeRequest() : os::Message(ID) {}
        };

        struct GetTimeResponse : public os::Message
        {
            static constexpr auto ID = device::message::Id::CLOCK_GET_TIME_RESPONSE;

            etl::optional<Timestamp> timestamp;

            GetTimeResponse() : os::Message(ID) {}
        };
    };
}

