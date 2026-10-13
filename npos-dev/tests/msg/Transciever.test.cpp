/**
 * @file Transciever.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Transciever.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Transciever)
{
    uint8_t buffer[8];
};

TEST(Transciever, ReadUsesOutputBuffer)
{
    npos::dev::msg::Transciever request({ 1, 2 }, npos::dev::msg::Transciever::READ, 4, etl::span<uint8_t>(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::dev::msg::Type::TRANSCIEVER, request.type);
    CHECK_EQUAL(npos::dev::msg::Transciever::READ, request.getOperation());
    CHECK_EQUAL(4, request.getInputLength());
    POINTERS_EQUAL(buffer, request.getOutputBuffer().data());
    CHECK_EQUAL(4, request.getOutputBuffer().size());
}

TEST(Transciever, WriteUsesInputBuffer)
{
    npos::dev::msg::Transciever request({ 1, 2 }, npos::dev::msg::Transciever::WRITE, 6, etl::span<uint8_t>(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::dev::msg::Transciever::WRITE, request.getOperation());
    POINTERS_EQUAL(buffer, request.getInputBuffer().data());
    CHECK_EQUAL(6, request.getInputBuffer().size());
}

TEST(Transciever, ResponseCarriesStatusAndLength)
{
    npos::dev::msg::Transciever request({ 1, 2 }, npos::dev::msg::Transciever::READ, 4, etl::span<uint8_t>(buffer, sizeof(buffer)));

    request.setStatus(npos::dev::msg::Transciever::BUSY).setOutputLength(3);

    CHECK_EQUAL(npos::dev::msg::Transciever::BUSY, request.getStatus());
    CHECK_EQUAL(3, request.getOutputLength());
}
