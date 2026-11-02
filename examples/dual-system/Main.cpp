/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-09-15
 */

#include <cassert>

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/dev/service/RtcClock.hpp>
#include <libnpos/nps/Scheduler.hpp>

#include <libnpos/emulator/Rtc.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	static constexpr npos::nps::Task::Priority DEVICE_TASK_PRIORITY = 2;
	static constexpr npos::nps::Task::Priority USER_TASK_PRIORITY = 1;

	static constexpr npos::ipc::PortId PRIMARY_BUS_START_ID = 0;
	static constexpr npos::ipc::PortId SECONDARY_BUS_START_ID = 128;

	//kernel primary
	npos::nps::SchedulerStatic<32> primaryScheduler;
	npos::ipc::SystemBusStatic<32, 64> primarySystemBus(PRIMARY_BUS_START_ID);

	//kernel secondary
	npos::nps::SchedulerStatic<32> secondaryScheduler;
	npos::ipc::SystemBusStatic<32, 64> secondarySystemBus(SECONDARY_BUS_START_ID);

	//connect kernels
	primaryScheduler.setSuccesor(secondaryScheduler);
	primarySystemBus.setSuccesor(secondarySystemBus);
	secondarySystemBus.setSuccesor(primarySystemBus);
	
	//Devices
	npos::ipc::ServiceTaskStatic<32, 32, 1> deviceTask(DEVICE_TASK_PRIORITY, primarySystemBus);
	assert(primarySystemBus.addPort(deviceTask));
	assert(primaryScheduler.addTask(deviceTask));

	npos::emulator::Rtc rtc;
	npos::dev::service::RtcClock rtcClock(deviceTask, rtc, "clock");
	assert(deviceTask.subscribe(rtcClock));

	//user app
	npos::ipc::ServiceTaskStatic<16, 16, 1> userTask(USER_TASK_PRIORITY, secondarySystemBus);
	assert(secondaryScheduler.addTask(userTask));
	assert(secondarySystemBus.addPort(userTask));

	UserApplication userApp(userTask, "clock");
	assert(userTask.subscribe(userApp));

	primaryScheduler.start();

	return 0;
}
