/**
 * @file Close.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/file/Close.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(FileClose)
{
};

TEST(FileClose, IsAddressedToTheFile)
{
    npos::fs::msg::file::Close request({ 3, 4 });

    CHECK_EQUAL(npos::fs::msg::Type::FILE_CLOSE, request.type);
    CHECK_EQUAL(3, request.getFile().portId);
    CHECK_EQUAL(4, request.getFile().serviceId);
}

TEST(FileClose, ResponseCarriesStatus)
{
    npos::fs::msg::file::Close request({ 3, 4 });

    request.setStatus(npos::fs::msg::Status::BAD_FILE_NUMBER);

    CHECK_EQUAL(npos::fs::msg::Status::BAD_FILE_NUMBER, request.getStatus());
}
