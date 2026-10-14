/**
 * @file Type.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Type.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(DevType)
{
};

TEST(DevType, TypesStartInDeviceRange)
{
    CHECK_EQUAL(npos::ipc::DEVICE_MESSAGE_TYPE, npos::dev::msg::Type::OPEN);
}

TEST(DevType, TypesAreConsecutive)
{
    using namespace npos::dev::msg;

    CHECK_EQUAL(Type::OPEN + 1, Type::CLOCK);
    CHECK_EQUAL(Type::CLOCK + 1, Type::LED);
    CHECK_EQUAL(Type::LED + 1, Type::HASH);
    CHECK_EQUAL(Type::HASH + 1, Type::BACKUP_REGISTERS);
    CHECK_EQUAL(Type::BACKUP_REGISTERS + 1, Type::TRANSCIEVER);
    CHECK_EQUAL(Type::TRANSCIEVER + 1, Type::TIMER);
}
