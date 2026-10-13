/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/UartInterrupt.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::UartInterrupt uart;

	npos::os::UserApi api;
	UserApplication userApp(api, "uart0");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::System::Hal hal;
	hal.uart.hal = &uart;

	npos::os::System system(hal, services, api);
	system.start();

	return 0;
}
