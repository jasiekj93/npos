/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-09-15
 */

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/dev/service/RtcClock.hpp>
#include <libnpos/nps/Scheduler.hpp>

#include "mock/Rtc.hpp"
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	//scheduler
	etl::vector<npos::nps::Task*, 32> taskList;
	npos::nps::Scheduler scheduler(taskList);

	//system bus
	etl::vector<npos::ipc::Port*, 32> portList;
	npos::ipc::SystemBus systemBus(portList, 0);

	//devices
	etl::vector<npos::ipc::Service*, 32> devServiceList;
	etl::queue<npos::ipc::Message, 32> devMessageQueue;
	npos::ipc::ServiceTask devTask(5, devServiceList, devMessageQueue, systemBus);
	systemBus.addPort(devTask);
	scheduler.addTask(devTask);

	mock::Rtc rtc;
	npos::dev::service::RtcClock rtcClock(devTask, rtc, "clock");
	devTask.subscribe(rtcClock);



	//userspace
	etl::vector<npos::ipc::Service*, 32> userServiceList;
	etl::queue<npos::ipc::Message, 32> userMessageQueue;
	npos::ipc::ServiceTask userTask(1, userServiceList, userMessageQueue, systemBus);
	systemBus.addPort(userTask);
	scheduler.addTask(userTask);

	UserApplication userApp(userTask, "clock");
	userTask.subscribe(userApp);

	scheduler.start();

	return 0;
}
