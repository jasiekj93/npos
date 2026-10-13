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
	npos::emulator::Led led0;
	npos::emulator::SystemTick systemTick;
	npos::dev::hal::Led* leds[] = { &led0 };

	npos::os::UserApi api;
	UserApplication userApp(api, "leds");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::System::Hal hal;
	hal.leds = leds;
	hal.systemTick = &systemTick;

	npos::os::System system(hal, services, api);
	system.start();

	return 0;
}
