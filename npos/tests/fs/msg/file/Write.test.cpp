/**
 * @file Write.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/file/Write.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(FileWrite)
{
    const uint8_t data[4] = { 1, 2, 3, 4 };
};

TEST(FileWrite, StoresFileAndData)
{
    npos::fs::msg::file::Write request({ 1, 2 }, etl::span<const uint8_t>(data, sizeof(data)));

    CHECK_EQUAL(npos::fs::msg::Type::FILE_WRITE, request.type);
    CHECK_EQUAL(2, request.getFile().serviceId);
    CHECK_EQUAL(4, request.getInputLength());
    POINTERS_EQUAL(data, request.getBuffer().data());
    CHECK_EQUAL(4, request.getBuffer().size());
}

TEST(FileWrite, ResponseCarriesLengthAndStatus)
{
    npos::fs::msg::file::Write request({ 1, 2 }, etl::span<const uint8_t>(data, sizeof(data)));

    request.setOutputLength(4).setStatus(npos::fs::msg::Status::NO_SPACE_LEFT);

    CHECK_EQUAL(4, request.getOutputLength());
    CHECK_EQUAL(npos::fs::msg::Status::NO_SPACE_LEFT, request.getStatus());
}
