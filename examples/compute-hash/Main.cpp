/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <cassert>

#include <libnpos/nps/Scheduler.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/dev/InterruptServiceTask.hpp>
#include <libnpos/dev/service/HashProcessor.hpp>

#include <libnpos/emulator/Hash.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	static constexpr npos::nps::Task::Priority DEVICE_TASK_PRIORITY = 2;
	static constexpr npos::nps::Task::Priority USER_TASK_PRIORITY = 1;

	//kernel
	npos::nps::SchedulerStatic<16> scheduler;
	npos::ipc::SystemBusStatic<16, 64> systemBus(0);

	//Devices
	npos::dev::InterruptServiceTaskStatic<16, 16, 1> deviceTask(DEVICE_TASK_PRIORITY, systemBus);
	assert(scheduler.addTask(deviceTask));
	assert(systemBus.addPort(deviceTask));

	npos::emulator::Hash hash(true);
	npos::dev::InterruptService::InterruptQueue<32> hashQueue;
	npos::dev::service::HashProcessor hashService(deviceTask, hashQueue, hash, "hash");
	assert(deviceTask.subscribe(hashService));

	//user app
	npos::ipc::ServiceTaskStatic<16, 16, 1> userTask(USER_TASK_PRIORITY, systemBus);
	assert(scheduler.addTask(userTask));
	assert(systemBus.addPort(userTask));

	UserApplication userApp(userTask, "hash");
	assert(userTask.subscribe(userApp));

	scheduler.start();
	return 0;
}
