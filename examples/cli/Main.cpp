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
#include <libnpos/dev/task/Timer.hpp>
#include <libnpos/dev/InterruptServiceTask.hpp>
#include <libnpos/dev/service/UartInterrupt.hpp>
#include <libnpos/dev/service/RtcClock.hpp>
#include <libnpos/cli/task/Terminal.hpp>
#include <libnpos/cli/program/Cat.hpp>
#include <libnpos/cli/program/Cd.hpp>
#include <libnpos/cli/program/Date.hpp>
#include <libnpos/cli/program/Echo.hpp>
#include <libnpos/cli/program/Hexdump.hpp>
#include <libnpos/cli/program/Ls.hpp>
#include <libnpos/cli/program/Mkdir.hpp>
#include <libnpos/cli/program/Pwd.hpp>
#include <libnpos/cli/program/Random.hpp>
#include <libnpos/cli/program/Rm.hpp>
#include <libnpos/cli/program/Rmdir.hpp>
#include <libnpos/cli/program/Touch.hpp>

#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include <libnpos/emulator/ConsoleUart.hpp>
#include <libnpos/emulator/Rtc.hpp>
#include <libnpos/emulator/SystemTick.hpp>

int main(int argc, char* argv[])
{
	static constexpr npos::nps::Task::Priority CLI_TASK_PRIORITY = 1;
	static constexpr npos::nps::Task::Priority USER_TASK_PRIORITY = 1;
	static constexpr npos::nps::Task::Priority DEVICE_TASK_PRIORITY = 2;
	static constexpr npos::nps::Task::Priority TIMER_TASK_PRIORITY = 3;
	static constexpr npos::nps::Task::Priority FILESYSTEM_TASK_PRIORITY = 4;
	static constexpr npos::nps::Task::Priority UART_TASK_PRIORITY = 5;

	//kernel
	npos::nps::SchedulerStatic<16> scheduler;
	npos::ipc::SystemBusStatic<16, 64> systemBus(0);

	//Devices
	npos::ipc::ServiceTaskStatic<16, 16, 1> deviceTask(DEVICE_TASK_PRIORITY, systemBus);
	assert(scheduler.addTask(deviceTask));
	assert(systemBus.addPort(deviceTask));
	
	npos::emulator::Rtc rtc;
	npos::dev::service::RtcClock rtcService(deviceTask, rtc, "rtc");
	assert(deviceTask.subscribe(rtcService));

	//Timer
	npos::emulator::SystemTick systemTick;
	npos::dev::task::TimerStatic<16> timerTask(TIMER_TASK_PRIORITY, systemTick, systemBus);
	assert(scheduler.addTask(timerTask));
	assert(systemBus.addPort(timerTask));

	//Filesystem
	npos::emulator::Flash flash;
	npos::emulator::ICache icache;
	npos::fs::task::LittlefsStatic<32, 8> filesystemTask(FILESYSTEM_TASK_PRIORITY, systemBus, flash, &icache);
	assert(scheduler.addTask(filesystemTask));
	assert(systemBus.addPort(filesystemTask));

	//Uart
	npos::dev::InterruptServiceTaskStatic<16, 16, 1> uartTask(UART_TASK_PRIORITY, systemBus);
	assert(scheduler.addTask(uartTask));
	assert(systemBus.addPort(uartTask));

	npos::emulator::ConsoleUart uart;
	npos::dev::InterruptService::InterruptQueue<16> uartInterruptQueue;
	npos::dev::service::UartInterrupt uartService(uartTask, uartInterruptQueue, uart, "uart0");
	assert(uartTask.subscribe(uartService));

	//Cli
	npos::cli::task::TerminalStatic<16, 32, 64, 16> terminalTask(CLI_TASK_PRIORITY, systemBus, "uart0");
	assert(scheduler.addTask(terminalTask));
	assert(systemBus.addPort(terminalTask));

	npos::cli::program::Cat cat(terminalTask);
	assert(terminalTask.addProgram(cat));

	npos::cli::program::Cd cd(terminalTask);
	assert(terminalTask.addProgram(cd));

	npos::cli::program::Date date(terminalTask, "rtc");
	assert(terminalTask.addProgram(date));

	npos::cli::program::Echo echo(terminalTask);
	assert(terminalTask.addProgram(echo));

	npos::cli::program::Hexdump hexdump(terminalTask);
	assert(terminalTask.addProgram(hexdump));

	npos::cli::program::Ls ls(terminalTask);
	assert(terminalTask.addProgram(ls));

	npos::cli::program::Mkdir mkdir(terminalTask);
	assert(terminalTask.addProgram(mkdir));

	npos::cli::program::Pwd pwd(terminalTask);
	assert(terminalTask.addProgram(pwd));

	npos::cli::program::Random random(terminalTask);
	assert(terminalTask.addProgram(random));

	npos::cli::program::Rm rm(terminalTask);
	assert(terminalTask.addProgram(rm));

	npos::cli::program::Rmdir rmdir(terminalTask);
	assert(terminalTask.addProgram(rmdir));

	npos::cli::program::Touch touch(terminalTask);
	assert(terminalTask.addProgram(touch));


	scheduler.start();
	return 0;
}
