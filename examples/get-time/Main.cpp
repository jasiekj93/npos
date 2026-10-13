/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Rtc.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::Rtc rtc;

	npos::os::UserApi api;
	UserApplication userApp(api, "clock");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::System::Hal hal;
	hal.rtc.hal = &rtc;

	npos::os::System system(hal, services, api);
	system.start();

	return 0;
}
