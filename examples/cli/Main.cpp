/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include <libnpos/emulator/ConsoleUart.hpp>
#include <libnpos/emulator/SystemTick.hpp>

int main(int argc, char* argv[])
{
	npos::emulator::SystemTick systemTick;
	npos::os::Storage<> storage;
	npos::emulator::Flash flash;
	npos::emulator::ICache icache;
	npos::emulator::ConsoleUart uart;
	npos::dev::InterruptService::InterruptQueue<32> uartQueue;
	npos::os::InterruptDevice<npos::dev::hal::UartInterrupt> uarts[] = { { uart, "uart0", uartQueue } };

	npos::os::UserApi api;

	npos::os::Hal hal(systemTick);
	hal.flash = &flash;
	hal.icache = &icache;
	hal.uarts = uarts;

	npos::os::Config config;
	config.cli = true;
	config.terminalDevice = "uart0";

	npos::os::System system(hal, storage.memory(), etl::span<npos::ipc::Service* const>(), api, config);
	system.start();

	return 0;
}
