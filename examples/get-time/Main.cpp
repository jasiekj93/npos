/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Rtc.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::SystemTick systemTick;
	npos::os::Storage<> storage;
	npos::emulator::Rtc rtc;
	npos::os::Device<npos::dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };

	npos::os::UserApi api;
	UserApplication userApp(api, "clock");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::Hal hal(systemTick);
	hal.rtcs = rtcs;

	npos::os::System system(hal, storage.memory(), services, api);
	system.start();

	return 0;
}
