/**
 * @file Hash.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Hash.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Hash)
{
    const uint8_t input[5] = { 1, 2, 3, 4, 5 };
    uint8_t output[32];
};

TEST(Hash, AccumulateCarriesOnlyInput)
{
    npos::dev::msg::Hash request({ 1, 2 }, npos::dev::msg::Hash::ACCUMULATE, etl::span<const uint8_t>(input, sizeof(input)));

    CHECK_EQUAL(npos::dev::msg::Type::HASH, request.type);
    CHECK_EQUAL(npos::dev::msg::Hash::ACCUMULATE, request.getOperation());
    POINTERS_EQUAL(input, request.getInput().data());
    CHECK_EQUAL(5, request.getInput().size());
}

TEST(Hash, AccumulateEndCarriesOutputBuffer)
{
    npos::dev::msg::Hash request({ 1, 2 }, npos::dev::msg::Hash::ACCUMULATE_END,
        etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(output, sizeof(output)));

    CHECK_EQUAL(npos::dev::msg::Hash::ACCUMULATE_END, request.getOperation());
    POINTERS_EQUAL(output, request.getOutput().data());
    CHECK_EQUAL(32, request.getOutput().size());
}

TEST(Hash, ResponseCarriesStatus)
{
    npos::dev::msg::Hash request({ 1, 2 }, npos::dev::msg::Hash::ACCUMULATE, etl::span<const uint8_t>(input, sizeof(input)));

    request.setStatus(npos::dev::msg::Hash::BUSY);

    CHECK_EQUAL(npos::dev::msg::Hash::BUSY, request.getStatus());
}
