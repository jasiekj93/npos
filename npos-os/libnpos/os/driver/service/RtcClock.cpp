#include "RtcClock.hpp"

using namespace npos;
using namespace npos::os;
using namespace npos::os::driver;
using namespace npos::os::driver::service;

RtcClock::RtcClock(hal::Rtc& rtc, os::message::Bus& bus)
    : Service(bus)
    , rtc(rtc)
{
}