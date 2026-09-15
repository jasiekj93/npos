#pragma once

/**
 * @file Rtc.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <cstdint>

namespace npos::os::driver::hal
{
    class Rtc
    {
    public:
        struct Time
        {
            uint8_t hours;   //0-23
            uint8_t minutes; //0-59
            uint8_t seconds; //0-59
        };

        struct Date
        {
            uint8_t day; //1-31
            uint8_t month; //1-12
            uint8_t year; //0-99 == 2000-2099
        };

        virtual ~Rtc() = default;

        virtual bool setTime(const Time &time) = 0;
        virtual bool getTime(Time &time) = 0;
        virtual bool setDate(const Date &date) = 0;
        virtual bool getDate(Date &date) = 0;
    };
}