#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 * @brief 
 * @details
 */

#include <libnpos/os/Timestamp.hpp>
#include <libnpos/os/message/RequestResponse.hpp>
#include <libnpos/os/driver/MessageId.hpp>
#include <libnpos/os/driver/hal/Rtc.hpp>

namespace npos::os::driver
{
    class Clock : public ipc::MessageRouter
    {
    public:
        struct SetTimeRequest : public os::message::Request
        {
            static constexpr ipc::Message::Id ID = MessageId::CLOCK_SET_TIME_REQUEST;

            Timestamp timestamp;

            SetTimeRequest() : os::message::Request(ID) {}
        };

        struct SetTimeResponse : public os::message::Response
        {
            static constexpr ipc::Message::Id ID = MessageId::CLOCK_SET_TIME_RESPONSE;

            enum Status
            {
                OK,
                RTC_ERROR,
                INVALID_TIMESTAMP
            };

            Status status;
            Timestamp timestamp;

            SetTimeResponse() : os::message::Response(ID) {}
        };

        struct GetTimeRequest : public os::message::Request
        {
            static constexpr ipc::Message::Id ID = MessageId::CLOCK_GET_TIME_REQUEST;

            GetTimeRequest() : os::message::Request(ID) {}
        };

        struct GetTimeResponse : public os::message::Response
        {
            static constexpr ipc::Message::Id ID = MessageId::CLOCK_GET_TIME_RESPONSE;

            bool status;
            Timestamp timestamp;

            GetTimeResponse() : os::message::Response(ID) {}
        };

    protected:
        void onReceive(const ipc::Message&) override;
        bool accepts(ipc::Message::Id) override;
    };
}

