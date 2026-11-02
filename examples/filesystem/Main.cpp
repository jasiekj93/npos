/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <cassert>

#include <libnpos/nps/Scheduler.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/fs/task/Littlefs.hpp>

#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	static constexpr npos::nps::Task::Priority USER_TASK_PRIORITY = 1;
	static constexpr npos::nps::Task::Priority FILESYSTEM_TASK_PRIORITY = 2;

	//kernel
	npos::nps::SchedulerStatic<16> scheduler;
	npos::ipc::SystemBusStatic<16, 64> systemBus(0);

	//Filesystem
	npos::emulator::Flash flash(true);
	npos::emulator::ICache icache(true);
	npos::fs::task::LittlefsStatic<32, 8> filesystemTask(FILESYSTEM_TASK_PRIORITY, systemBus, flash, &icache);
	assert(scheduler.addTask(filesystemTask));
	assert(systemBus.addPort(filesystemTask));

	//user app
	npos::ipc::ServiceTaskStatic<16, 16, 1> userTask(USER_TASK_PRIORITY, systemBus);
	assert(scheduler.addTask(userTask));
	assert(systemBus.addPort(userTask));

	UserApplication userApp(userTask);
	assert(userTask.subscribe(userApp));

	scheduler.start();
	return 0;
}
