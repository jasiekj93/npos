#pragma once

/**
 * @file UartInterrupt.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <iostream>

#include <libnpos/dev/hal/Uart.hpp>

namespace mock
{
    class UartInterrupt : public npos::dev::hal::UartInterrupt
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

            std::cout << "Emulator: UartInterrupt transmitIt, size: " << size << std::endl;

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::UART, Event::TRANSMIT_COMPLETE});
            return true;
        }

        bool receiveIt(uint8_t* data, size_t size) override
        {
            if(not initialized)
                return false;

            std::cout << "Emulator: UartInterrupt receiveIt, size: " << size << std::endl;

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::UART, Event::RECEIVE_COMPLETE});
            return true;
        }

        void enableInterrupts() override
        {
            // std::cout << "Emulator: Hash enableInterrupts" << std::endl;
        }

        void disableInterrupts() override
        {
            // std::cout << "Emulator: Hash disableInterrupts" << std::endl;
        }

        void setHandler(npos::dev::hal::InterruptHandler* handler) override
        {
            interruptHandler = handler;
        }

        bool initialized = false;
        npos::dev::hal::InterruptHandler* interruptHandler = nullptr;
    };
}