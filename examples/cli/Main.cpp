/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include <libnpos/emulator/ConsoleUart.hpp>

int main(int argc, char* argv[])
{
	npos::emulator::Flash flash;
	npos::emulator::ICache icache;
	npos::emulator::ConsoleUart uart;

	npos::os::System::Hal hal;
	hal.flash = &flash;
	hal.icache = &icache;
	hal.uart.hal = &uart;

	npos::os::System::Config config;
	config.cli = true;
	config.terminalDevice = "uart0";

	npos::os::UserApi api;
	npos::os::System system(hal, {}, api, config);
	system.start();

	return 0;
}
