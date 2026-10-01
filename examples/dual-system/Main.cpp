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
	etl::vector<npos::nps::Task*, 32> primaryTaskList;
	npos::nps::Scheduler primaryScheduler(primaryTaskList);
	etl::vector<npos::nps::Task*, 32> secondaryTaskList;
	npos::nps::Scheduler secondaryScheduler(secondaryTaskList);
	primaryScheduler.setSuccesor(secondaryScheduler);

	//system bus
	etl::vector<npos::ipc::Port*, 32> primaryPortList;
	npos::ipc::SystemBus primarySystemBus(primaryPortList, 0);
	etl::vector<npos::ipc::Port*, 32> secondaryPortList;
	npos::ipc::SystemBus secondarySystemBus(secondaryPortList, 128);
	primarySystemBus.setSuccesor(secondarySystemBus);
	secondarySystemBus.setSuccesor(primarySystemBus);

	//devices
	etl::vector<npos::ipc::Service*, 32> devServiceList;
	etl::queue<npos::ipc::Message, 32> devMessageQueue;
	npos::ipc::ServiceTask devTask(5, devServiceList, devMessageQueue, primarySystemBus);
	primarySystemBus.addPort(devTask);
	primaryScheduler.addTask(devTask);

	mock::Rtc rtc;
	npos::dev::service::RtcClock rtcClock(devTask, rtc, "clock");
	devTask.subscribe(rtcClock);



	//userspace
	etl::vector<npos::ipc::Service*, 32> userServiceList;
	etl::queue<npos::ipc::Message, 32> userMessageQueue;
	npos::ipc::ServiceTask userTask(1, userServiceList, userMessageQueue, secondarySystemBus);
	secondarySystemBus.addPort(userTask);
	secondaryScheduler.addTask(userTask);

	UserApplication userApp(userTask, "clock");
	userTask.subscribe(userApp);

	primaryScheduler.start();

	return 0;
}
