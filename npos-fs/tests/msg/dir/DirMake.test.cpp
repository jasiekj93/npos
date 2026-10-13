/**
 * @file DirMake.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/dir/Make.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(DirMake)
{
};

TEST(DirMake, StoresPath)
{
    npos::fs::msg::dir::Make request("/a/b");

    CHECK_EQUAL(npos::fs::msg::Type::DIR_MAKE, request.type);
    STRCMP_EQUAL("/a/b", request.getPath().data());
    CHECK_EQUAL(4, request.getPath().size());
}

TEST(DirMake, ResponseCarriesStatus)
{
    npos::fs::msg::dir::Make request("/a");

    request.setStatus(npos::fs::msg::Status::ENTRY_EXISTS);

    CHECK_EQUAL(npos::fs::msg::Status::ENTRY_EXISTS, request.getStatus());
}
