/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::Flash flash(true);
	npos::emulator::ICache icache(true);

	npos::os::UserApi api;
	UserApplication userApp(api);
	npos::ipc::Service* services[] = { &userApp };

	npos::os::System::Hal hal;
	hal.flash = &flash;
	hal.icache = &icache;

	npos::os::System system(hal, services, api);
	system.start();

	return 0;
}
