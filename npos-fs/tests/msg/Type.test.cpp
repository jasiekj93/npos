/**
 * @file Type.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/Type.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Type)
{
};

TEST(Type, FileTypesStartInFilesystemRange)
{
    CHECK_EQUAL(npos::ipc::FILESYSTEM_MESSAGE_TYPE, npos::fs::msg::Type::FILE_OPEN);
}

TEST(Type, TypesAreConsecutiveAndDistinct)
{
    CHECK_EQUAL(npos::fs::msg::Type::FILE_OPEN + 1, npos::fs::msg::Type::FILE_CLOSE);
    CHECK_EQUAL(npos::fs::msg::Type::FILE_CLOSE + 1, npos::fs::msg::Type::FILE_READ);
    CHECK_EQUAL(npos::fs::msg::Type::FILE_READ + 1, npos::fs::msg::Type::FILE_WRITE);
    CHECK_EQUAL(npos::fs::msg::Type::FILE_WRITE + 1, npos::fs::msg::Type::DIR_OPEN);
    CHECK_EQUAL(npos::fs::msg::Type::DIR_OPEN + 1, npos::fs::msg::Type::DIR_CLOSE);
    CHECK_EQUAL(npos::fs::msg::Type::DIR_CLOSE + 1, npos::fs::msg::Type::DIR_READ);
}
