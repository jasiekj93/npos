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
	npos::nps::Scheduler::TaskList<32> primaryTaskList;
	npos::nps::Scheduler primaryScheduler(primaryTaskList);
	npos::nps::Scheduler::TaskList<32> secondaryTaskList;
	npos::nps::Scheduler secondaryScheduler(secondaryTaskList);
	primaryScheduler.setSuccesor(secondaryScheduler);

	//system bus
	npos::ipc::SystemBus::PortList<32> primaryPortList;
	etl::pool<npos::ipc::Message, 64> primaryMessagePool;
	npos::ipc::SystemBus primarySystemBus(primaryPortList, primaryMessagePool, 0);
	npos::ipc::SystemBus::PortList<32> secondaryPortList;
	etl::pool<npos::ipc::Message, 64> secondaryMessagePool;
	npos::ipc::SystemBus secondarySystemBus(secondaryPortList, secondaryMessagePool, 128);
	primarySystemBus.setSuccesor(secondarySystemBus);
	secondarySystemBus.setSuccesor(primarySystemBus);

	//devices
	npos::ipc::ServiceTask::ServiceList<32> devServiceList;
	npos::ipc::ServiceTask::MessageQueue<32> devMessageQueue;
	npos::ipc::ServiceTask devTask(5, devServiceList, devMessageQueue, primarySystemBus);
	primarySystemBus.addPort(devTask);
	primaryScheduler.addTask(devTask);

	mock::Rtc rtc;
	npos::dev::service::RtcClock rtcClock(devTask, rtc, "clock");
	devTask.subscribe(rtcClock);



	//userspace
	npos::ipc::ServiceTask::ServiceList<32> userServiceList;
	npos::ipc::ServiceTask::MessageQueue<32> userMessageQueue;
	npos::ipc::ServiceTask userTask(1, userServiceList, userMessageQueue, secondarySystemBus);
	secondarySystemBus.addPort(userTask);
	secondaryScheduler.addTask(userTask);

	UserApplication userApp(userTask, "clock");
	userTask.subscribe(userApp);

	primaryScheduler.start();

	return 0;
}
