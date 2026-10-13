/**
 * @file Flags.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/Flags.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Flags)
{
};

TEST(Flags, ReadWriteIsReadAndWrite)
{
    CHECK_EQUAL(npos::fs::Flags::READ_ONLY | npos::fs::Flags::WRITE_ONLY, npos::fs::Flags::READ_WRITE);
}

TEST(Flags, ModifiersDoNotOverlapAccessModes)
{
    auto modifiers = npos::fs::Flags::CREATE | npos::fs::Flags::EXCLUSIVE | npos::fs::Flags::TRUNCATE | npos::fs::Flags::APPEND;

    CHECK_EQUAL(0, modifiers & npos::fs::Flags::READ_WRITE);
}

TEST(Flags, ModifiersAreSeparateBits)
{
    CHECK_EQUAL(0x0F00, npos::fs::Flags::CREATE | npos::fs::Flags::EXCLUSIVE | npos::fs::Flags::TRUNCATE | npos::fs::Flags::APPEND);
}
