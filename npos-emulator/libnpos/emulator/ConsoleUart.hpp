#pragma once

/**
 * @file ConsoleUart.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include <libnpos/dev/hal/Uart.hpp>

namespace npos::emulator
{
    // Uart backed by stdin/stdout; receiving blocks until input arrives and ends the program on EOF.
    class ConsoleUart : public npos::dev::hal::UartInterrupt
    {
    public:
        bool init() override
        {
            initialized = true;
            return true;
        }

        void deinit() override
        {
        }

        bool transmitIt(const uint8_t* data, size_t size) override 
        {
            if(not initialized)
                return false;

            std::fwrite(data, 1, size, stdout);
            std::fflush(stdout);

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::UART, Event::TRANSMIT_COMPLETE});
            return true;
        }

        bool receiveIt(uint8_t* data, size_t size) override
        {
            if(not initialized)
                return false;

            // Returns as soon as any input arrives, the rest of the buffer is zero filled.
            auto received = ::read(STDIN_FILENO, data, size);

            if(received <= 0)
                std::exit(0);

            std::memset(data + received, 0, size - static_cast<size_t>(received));

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::UART, Event::RECEIVE_COMPLETE});
            return true;
        }

        void enableInterrupts() override
        {
        }

        void disableInterrupts() override
        {
        }

        void setHandler(npos::dev::hal::InterruptHandler* handler) override
        {
            interruptHandler = handler;
        }

        bool initialized = false;
        npos::dev::hal::InterruptHandler* interruptHandler = nullptr;
    };
}
