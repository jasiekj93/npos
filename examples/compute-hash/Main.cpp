/**
 * @file Main.cpp
 * @author Adrian Szczepanski
 * @date 2026-10-13
 */

#include <libnpos/os/System.hpp>
#include <libnpos/emulator/Hash.hpp>
#include <libnpos/emulator/SystemTick.hpp>
#include "UserApplication.hpp"

int main(int argc, char* argv[])
{
	npos::emulator::SystemTick systemTick;
	npos::os::Storage<> storage;
	npos::emulator::Hash hash(true);
	npos::dev::InterruptService::InterruptQueue<32> hashQueue;
	npos::os::InterruptDevice<npos::dev::hal::Hash> hashes[] = { { hash, "hash", hashQueue } };

	npos::os::UserApi api;
	UserApplication userApp(api, "hash");
	npos::ipc::Service* services[] = { &userApp };

	npos::os::Hal hal(systemTick);
	hal.hashes = hashes;

	npos::os::System system(hal, storage.memory(), services, api);
	system.start();

	return 0;
}
