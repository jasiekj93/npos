#pragma once

#include <libnpos/dev/hal/Rtc.hpp>

namespace mock
{
    class Rtc : public npos::dev::hal::Rtc
    {
    public:
        bool initalize() override
        {
            return true;
        }

        bool setTime(const Time& time) override
        {
            this->time = time;
            return true;
        }

        bool getTime(Time& time) override
        {
            time = this->time;
            return true;
        }

        bool setDate(const Date& date) override
        {
            this->date = date;
            return true;
        }

        bool getDate(Date& date) override
        {
            date = this->date;
            return true;
        }

        Time time { 12, 50, 24 };
        Date date { 29, 9, 26 };
    };
}