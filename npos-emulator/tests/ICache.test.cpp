/**
 * @file ICache.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/ICache.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorICache)
{
    npos::emulator::ICache icache;
};

TEST(EmulatorICache, AllOperationsSucceed)
{
    CHECK_TRUE(icache.init());
    CHECK_TRUE(icache.enable());
    CHECK_TRUE(icache.disable());
}

TEST(EmulatorICache, VerboseModeDoesNotChangeBehaviour)
{
    npos::emulator::ICache verbose(true);

    CHECK_TRUE(verbose.init());
    CHECK_TRUE(verbose.enable());
    CHECK_TRUE(verbose.disable());
}
