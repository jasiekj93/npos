/**
 * @file Status.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/Status.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Status)
{
};

TEST(Status, SuccessIsZero)
{
    CHECK_EQUAL(0, npos::fs::msg::Status::SUCCESS);
}

TEST(Status, EndOfDirectoryIsDistinctFromErrors)
{
    CHECK_TRUE(npos::fs::msg::Status::END_OF_DIRECTORY != npos::fs::msg::Status::SUCCESS);
    CHECK_TRUE(npos::fs::msg::Status::END_OF_DIRECTORY != npos::fs::msg::Status::UNKNOWN_ERROR);
}
