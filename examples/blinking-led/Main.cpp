/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Led.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::SystemTick systemTick;
	npos::os::Storage<> storage;
	npos::emulator::Led led0(true);
	etl::vector<npos::dev::hal::Led*, 1> leds { &led0 };
	npos::os::LedBank banks[] = { { leds, "leds" } };

	npos::os::UserApi api;
	UserApplication userApp(api, "leds");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::Hal hal(systemTick);
	hal.leds = banks;

	npos::os::System system(hal, storage.memory(), services, api);
	system.start();

	return 0;
}
