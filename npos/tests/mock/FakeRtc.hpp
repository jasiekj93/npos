#pragma once

/**
 * @file FakeRtc.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/Rtc.hpp>

namespace mock
{
    class FakeRtc : public npos::dev::hal::Rtc
    {
    public:
        bool initialize() override { initCount++; return not failInit; }
        bool setTime(const Time& value) override { time = value; return not failSetTime; }
        bool getTime(Time& value) override { value = time; return not failGetTime; }
        bool setDate(const Date& value) override { date = value; return not failSetDate; }
        bool getDate(Date& value) override { value = date; return not failGetDate; }

        Time time { 0, 0, 0 };
        Date date { 1, 1, 0 };
        bool failInit = false;
        bool failSetTime = false;
        bool failGetTime = false;
        bool failSetDate = false;
        bool failGetDate = false;
        int initCount = 0;
    };
}
