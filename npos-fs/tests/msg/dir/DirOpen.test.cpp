/**
 * @file DirOpen.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/dir/Open.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(DirOpen)
{
};

TEST(DirOpen, StoresPath)
{
    npos::fs::msg::dir::Open request("/etc");

    CHECK_EQUAL(npos::fs::msg::Type::DIR_OPEN, request.type);
    STRCMP_EQUAL("/etc", request.getPath().data());
    CHECK_EQUAL(4, request.getPath().size());
}

TEST(DirOpen, ResponseCarriesStatusAndOpenedOid)
{
    npos::fs::msg::dir::Open request("/");

    request.setStatus(npos::fs::msg::Status::NOT_A_DIRECTORY).setOpened({ 1, 6 });

    CHECK_EQUAL(npos::fs::msg::Status::NOT_A_DIRECTORY, request.getStatus());
    CHECK_EQUAL(6, request.getOpened().serviceId);
}
