/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/dev/InterruptServiceTask.hpp>
#include <libnpos/dev/service/UartInterrupt.hpp>
#include <libnpos/fs/task/Littlefs.hpp>
#include <libnpos/nps/Scheduler.hpp>
#include <libnpos/cli/service/Terminal.hpp>
#include <libnpos/cli/program/Cat.hpp>
#include <libnpos/cli/program/Echo.hpp>

#include "mock/Flash.hpp"
#include "mock/ICache.hpp"
#include "mock/UartInterrupt.hpp"

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
	npos::dev::InterruptServiceTask::InterruptServiceList<32> devServiceList;
	npos::dev::InterruptServiceTask::MessageQueue<32> devMessageQueue;
	npos::dev::InterruptServiceTask devTask(5, devServiceList, devMessageQueue, systemBus);
	systemBus.addPort(devTask);
	scheduler.addTask(devTask);

	mock::UartInterrupt uart;
	npos::dev::InterruptService::InterruptQueue<32> uartInterruptQueue;
	npos::dev::service::UartInterrupt uartHandler(devTask, uartInterruptQueue, uart, "uart0");
	devTask.subscribe(uartHandler);

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

	npos::cli::service::Terminal::ProgramList<8> programList;
	npos::cli::service::Terminal terminal(userTask, programList, "uart0");
	userTask.subscribe(terminal);

	npos::cli::program::Cat cat(userTask);
	npos::cli::program::Echo echo(userTask);

	terminal.addProgram(cat);
	terminal.addProgram(echo);

	scheduler.start();

	return 0;
}
