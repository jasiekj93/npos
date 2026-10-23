#include <CppUTest/TestHarness.h>

#include <libnpos/emulator/InterconnectUart.hpp>

namespace
{
    class TestInterruptHandler : public npos::dev::hal::InterruptHandler
    {
    public:
        void handleInterrupt(const npos::dev::hal::Interrupt& interrupt) override
        {
            lastInterrupt = interrupt;
            ++interruptCount;
        }

        npos::dev::hal::Interrupt lastInterrupt{};
        size_t interruptCount = 0;
    };
}

TEST_GROUP(InterconnectUart)
{
    npos::emulator::InterconnectUart first;
    npos::emulator::InterconnectUart second;
    TestInterruptHandler firstHandler;
    TestInterruptHandler secondHandler;

    void setup() override
    {
        first.init();
        second.init();
        first.setHandler(&firstHandler);
        second.setHandler(&secondHandler);
        CHECK_TRUE(first.connect(second));
    }
};

TEST(InterconnectUart, TransmitCompletesBothEndpointsPendingRead)
{
    uint8_t received[4] = {};
    const uint8_t sent[] = {1, 2, 3};

    CHECK_TRUE(second.receiveIt(received, sizeof(received)));
    CHECK_TRUE(first.transmitIt(sent, sizeof(sent)));

    CHECK_EQUAL(1, firstHandler.interruptCount);
    CHECK_EQUAL(npos::dev::hal::UartInterrupt::Event::TRANSMIT_COMPLETE, firstHandler.lastInterrupt.code);
    CHECK_EQUAL(1, secondHandler.interruptCount);
    CHECK_EQUAL(npos::dev::hal::UartInterrupt::Event::RECEIVE_COMPLETE, secondHandler.lastInterrupt.code);
    CHECK_EQUAL(1, received[0]);
    CHECK_EQUAL(2, received[1]);
    CHECK_EQUAL(3, received[2]);
    CHECK_EQUAL(0, received[3]);
}

TEST(InterconnectUart, BuffersTransmittedBytesUntilRead)
{
    const uint8_t sent[] = {4, 5, 6};
    CHECK_TRUE(first.transmitIt(sent, sizeof(sent)));
    CHECK_EQUAL(0, secondHandler.interruptCount);

    uint8_t firstRead[2] = {};
    CHECK_TRUE(second.receiveIt(firstRead, sizeof(firstRead)));
    CHECK_EQUAL(1, secondHandler.interruptCount);
    CHECK_EQUAL(4, firstRead[0]);
    CHECK_EQUAL(5, firstRead[1]);

    uint8_t secondRead[2] = {};
    CHECK_TRUE(second.receiveIt(secondRead, sizeof(secondRead)));
    CHECK_EQUAL(2, secondHandler.interruptCount);
    CHECK_EQUAL(6, secondRead[0]);
    CHECK_EQUAL(0, secondRead[1]);
}

TEST(InterconnectUart, ConnectsBothDirections)
{
    uint8_t received = 0;
    const uint8_t sent = 7;

    CHECK_TRUE(first.receiveIt(&received, 1));
    CHECK_TRUE(second.transmitIt(&sent, 1));

    CHECK_EQUAL(sent, received);
    CHECK_EQUAL(npos::dev::hal::UartInterrupt::Event::RECEIVE_COMPLETE, firstHandler.lastInterrupt.code);
    CHECK_EQUAL(npos::dev::hal::UartInterrupt::Event::TRANSMIT_COMPLETE, secondHandler.lastInterrupt.code);
}

TEST(InterconnectUart, TransmitFailsWithoutConnectedInitializedPeer)
{
    npos::emulator::InterconnectUart unconnected;
    const uint8_t byte = 1;

    CHECK_FALSE(unconnected.transmitIt(&byte, 1));

    second.deinit();
    CHECK_FALSE(first.transmitIt(&byte, 1));
}
