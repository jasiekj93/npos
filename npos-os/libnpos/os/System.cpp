#include "System.hpp"

#include <libnpos/os/driver/service/RtcClock.hpp>

using namespace npos;
using namespace npos::os;

System::System(System::TaskStorage& taskStorage, System::DeviceList& devices)
    : scheduler(taskStorage)
    , devices(devices)
    , systemBus()
{

    if(devices.rtc.has_value())
    {
        //jakiś new albo sharedPrt;
        //albo coś?
        // driver::service::RtcClock rtcClock;
        // driver::service::RtcClockProcess rtcClockProcess(devices.rtc.value());
    }
}

bool npos::os::System::addProcess(os::Process &process)
{
    if(scheduler.full() or systemBus.full())
        return false;

    if(not scheduler.addTask(process))
        return false;

    if(not systemBus.subscribe(process))
        return false;

    return true;
}