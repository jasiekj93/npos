/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Hash.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::Hash hash;

	npos::os::UserApi api;
	UserApplication userApp(api, "hash");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::System::Hal hal;
	hal.hash.hal = &hash;

	npos::os::System system(hal, services, api);
	system.start();

	return 0;
}
