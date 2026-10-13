/**
 * @file Hash.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/emulator/Hash.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Handler : public npos::dev::hal::InterruptHandler
    {
    public:
        void handleInterrupt(const npos::dev::hal::Interrupt& interrupt) override { events.push_back(interrupt.code); }

        etl::vector<int, 4> events;
    };
}

TEST_GROUP(EmulatorHash)
{
    npos::emulator::Hash hash;
    Handler handler;
    uint8_t input[4] = {};
    uint8_t digest[npos::dev::hal::Hash::DIGEST_SIZE];

    void setup() override { hash.setHandler(&handler); }
};

TEST(EmulatorHash, FailsBeforeInit)
{
    CHECK_FALSE(hash.accumulateIt(etl::span<const uint8_t>(input, sizeof(input))));
    CHECK_FALSE(hash.accumulateEndIt(etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(digest, sizeof(digest))));
}

TEST(EmulatorHash, AccumulateRaisesInputCompleteInterrupt)
{
    hash.init();

    CHECK_TRUE(hash.accumulateIt(etl::span<const uint8_t>(input, sizeof(input))));

    CHECK_EQUAL(1, handler.events.size());
    CHECK_EQUAL(npos::dev::hal::Hash::INPUT_COMPLETE, handler.events[0]);
}

TEST(EmulatorHash, AccumulateEndRaisesDigestCompleteInterrupt)
{
    hash.init();

    CHECK_TRUE(hash.accumulateEndIt(etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(digest, sizeof(digest))));

    CHECK_EQUAL(npos::dev::hal::Hash::DIGEST_COMPLETE, handler.events[0]);
}

TEST(EmulatorHash, AccumulateEndRejectsTooSmallOutput)
{
    hash.init();

    CHECK_FALSE(hash.accumulateEndIt(etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(digest, 4)));
    CHECK_EQUAL(0, handler.events.size());
}

TEST(EmulatorHash, WorksWithoutHandler)
{
    hash.setHandler(nullptr);
    hash.init();

    CHECK_TRUE(hash.accumulateIt(etl::span<const uint8_t>(input, sizeof(input))));
}

TEST(EmulatorHash, InterruptControlAndDeinitAreCallable)
{
    hash.enableInterrupts();
    hash.disableInterrupts();
    hash.deinit();

    CHECK_EQUAL(0, handler.events.size());
}
