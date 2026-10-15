/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-09-15
 */

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/dev/service/RtcClock.hpp>
#include <libnpos/nps/Scheduler.hpp>

#include <libnpos/emulator/Rtc.hpp>
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
	npos::ipc::ServiceTask::RequestQueue<32> devRequestQueue;
	npos::ipc::ServiceTask::ResponseQueue<32> devResponseQueue;
	npos::ipc::ServiceTask devTask(5, devServiceList, devRequestQueue, devResponseQueue, primarySystemBus);
	primarySystemBus.addPort(devTask);
	primaryScheduler.addTask(devTask);

	npos::emulator::Rtc rtc;
	npos::dev::service::RtcClock rtcClock(devTask, rtc, "clock");
	devTask.subscribe(rtcClock);



	//userspace
	npos::ipc::ServiceTask::ServiceList<32> userServiceList;
	npos::ipc::ServiceTask::RequestQueue<32> userRequestQueue;
	npos::ipc::ServiceTask::ResponseQueue<32> userResponseQueue;
	npos::ipc::ServiceTask userTask(1, userServiceList, userRequestQueue, userResponseQueue, secondarySystemBus);
	secondarySystemBus.addPort(userTask);
	secondaryScheduler.addTask(userTask);

	UserApplication userApp(userTask, "clock");
	userTask.subscribe(userApp);

	primaryScheduler.start();

	return 0;
}
