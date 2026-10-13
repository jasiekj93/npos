/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/UartInterrupt.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::SystemTick systemTick;
	npos::os::Storage<> storage;
	npos::emulator::UartInterrupt uart(true);
	npos::dev::InterruptService::InterruptQueue<32> uartQueue;
	npos::os::InterruptDevice<npos::dev::hal::UartInterrupt> uarts[] = { { uart, "uart0", uartQueue } };

	npos::os::UserApi api;
	UserApplication userApp(api, "uart0");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::Hal hal(systemTick);
	hal.uarts = uarts;

	npos::os::System system(hal, storage.memory(), services, api);
	system.start();

	return 0;
}
