/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-09-15
 */

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/fs/task/Littlefs.hpp>
#include <libnpos/nps/Scheduler.hpp>

#include "mock/Flash.hpp"
#include "mock/ICache.hpp"
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

	//filesystem
	mock::Flash flash;
	mock::ICache icache;
	npos::fs::task::Littlefs<32>::MessageQueue<32> littlefsMessageQueue;
	npos::fs::task::Littlefs<32> littlefsTask(4, littlefsMessageQueue, systemBus, flash, &icache);
	systemBus.addPort(littlefsTask);
	scheduler.addTask(littlefsTask);

	//userspace
	npos::ipc::ServiceTask::ServiceList<32> userServiceList;
	npos::ipc::ServiceTask::MessageQueue<32> userMessageQueue;
	npos::ipc::ServiceTask userTask(1, userServiceList, userMessageQueue, systemBus);
	systemBus.addPort(userTask);
	scheduler.addTask(userTask);

	UserApplication userApp(userTask);
	userTask.subscribe(userApp);

	scheduler.start();

	return 0;
}
