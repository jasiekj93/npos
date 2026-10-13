/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Flash.hpp>
#include <libnpos/emulator/ICache.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::SystemTick systemTick;
	npos::os::Storage<> storage;
	npos::emulator::Flash flash(true);
	npos::emulator::ICache icache(true);

	npos::os::UserApi api;
	UserApplication userApp(api);
	npos::ipc::Service* services[] = { &userApp };

	npos::os::Hal hal(systemTick);
	hal.flash = &flash;
	hal.icache = &icache;

	npos::os::System system(hal, storage.memory(), services, api);
	system.start();

	return 0;
}
