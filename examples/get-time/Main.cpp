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
	npos::nps::Scheduler::TaskList<32> taskList;
	npos::nps::Scheduler scheduler(taskList);

	//system bus
	npos::ipc::SystemBus::PortList<32> portList;
	etl::pool<npos::ipc::Message, 64> messagePool;
	npos::ipc::SystemBus systemBus(portList, messagePool, 0);

	//devices
	npos::ipc::ServiceTask::ServiceList<32> devServiceList;
	npos::ipc::ServiceTask::MessageQueue<32> devMessageQueue;
	npos::ipc::ServiceTask devTask(5, devServiceList, devMessageQueue, systemBus);
	systemBus.addPort(devTask);
	scheduler.addTask(devTask);

	mock::Rtc rtc;
	npos::dev::service::RtcClock rtcClock(devTask, rtc, "clock");
	devTask.subscribe(rtcClock);



	//userspace
	npos::ipc::ServiceTask::ServiceList<32> userServiceList;
	npos::ipc::ServiceTask::MessageQueue<32> userMessageQueue;
	npos::ipc::ServiceTask userTask(1, userServiceList, userMessageQueue, systemBus);
	systemBus.addPort(userTask);
	scheduler.addTask(userTask);

	UserApplication userApp(userTask, "clock");
	userTask.subscribe(userApp);

	scheduler.start();

	return 0;
}
