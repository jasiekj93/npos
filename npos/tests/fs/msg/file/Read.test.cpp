/**
 * @file Read.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/file/Read.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(FileRead)
{
    uint8_t buffer[16];
};

TEST(FileRead, StoresFileLengthAndBuffer)
{
    npos::fs::msg::file::Read request({ 1, 2 }, 8, etl::span<uint8_t>(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::fs::msg::Type::FILE_READ, request.type);
    CHECK_EQUAL(2, request.getFile().serviceId);
    CHECK_EQUAL(8, request.getInputLength());
    POINTERS_EQUAL(buffer, request.getBuffer().data());
    CHECK_EQUAL(sizeof(buffer), request.getBuffer().size());
}

TEST(FileRead, ResponseCarriesLengthAndStatus)
{
    npos::fs::msg::file::Read request({ 1, 2 }, 8, etl::span<uint8_t>(buffer, sizeof(buffer)));

    request.setOutputLength(5).setStatus(npos::fs::msg::Status::SUCCESS);

    CHECK_EQUAL(5, request.getOutputLength());
    CHECK_EQUAL(npos::fs::msg::Status::SUCCESS, request.getStatus());
}
