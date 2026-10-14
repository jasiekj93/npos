/**
 * @file DirClose.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/dir/Close.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(DirClose)
{
};

TEST(DirClose, IsAddressedToTheDirectory)
{
    npos::fs::msg::dir::Close request({ 3, 1 });

    CHECK_EQUAL(npos::fs::msg::Type::DIR_CLOSE, request.type);
    CHECK_EQUAL(3, request.getDir().portId);
    CHECK_EQUAL(1, request.getDir().serviceId);
}

TEST(DirClose, ResponseCarriesStatus)
{
    npos::fs::msg::dir::Close request({ 3, 1 });

    request.setStatus(npos::fs::msg::Status::BAD_FILE_NUMBER);

    CHECK_EQUAL(npos::fs::msg::Status::BAD_FILE_NUMBER, request.getStatus());
}
