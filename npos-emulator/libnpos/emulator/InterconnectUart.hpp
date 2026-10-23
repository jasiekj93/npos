#pragma once

/**
 * @file InterconnectUart.hpp
 * @author Adrian Szczepanski
 * @date 23-10-2026
 */

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>

#include <libnpos/dev/hal/Uart.hpp>

namespace npos::emulator
{
    // Connect two UART endpoints for in-process communication.
    class InterconnectUart : public npos::dev::hal::UartInterrupt
    {
    public:
        bool init() override
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            initialized = true;
            return true;
        }

        void deinit() override
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            initialized = false;
            pendingData = nullptr;
            pendingSize = 0;
            receivedData.clear();
        }

        bool transmitIt(const uint8_t* data, size_t size) override
        {
            if(size > 0 and not data)
                return false;

            InterconnectUart* destination = nullptr;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                if(not initialized)
                    return false;
                destination = peer;
            }

            if(not destination or not destination->receiveBytes(data, size))
                return false;

            notifyInterrupt(Event::TRANSMIT_COMPLETE);
            return true;
        }

        bool receiveIt(uint8_t* data, size_t size) override
        {
            if(not data or size == 0)
                return false;

            bool complete = false;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                if(not initialized or pendingData)
                    return false;

                if(receivedData.empty())
                {
                    pendingData = data;
                    pendingSize = size;
                    return true;
                }

                copyReceivedData(data, size);
                complete = true;
            }

            if(complete)
                notifyInterrupt(Event::RECEIVE_COMPLETE);

            return true;
        }

        void enableInterrupts() override
        {
            interruptMutex.unlock();
        }

        void disableInterrupts() override
        {
            interruptMutex.lock();
        }

        void setHandler(npos::dev::hal::InterruptHandler* handler) override
        {
            std::lock_guard<std::mutex> lock(interruptMutex);
            interruptHandler = handler;
        }

        void setPeer(InterconnectUart* peer)
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            this->peer = peer;
        }

        bool connect(InterconnectUart& other)
        {
            if(&other == this)
                return false;

            setPeer(&other);
            other.setPeer(this);
            return true;
        }

    private:
        bool receiveBytes(const uint8_t* data, size_t size)
        {
            bool complete = false;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                if(not initialized)
                    return false;

                if(size > 0)
                    receivedData.insert(receivedData.end(), data, data + size);

                if(pendingData and not receivedData.empty())
                {
                    copyReceivedData(pendingData, pendingSize);
                    pendingData = nullptr;
                    pendingSize = 0;
                    complete = true;
                }
            }

            if(complete)
                notifyInterrupt(Event::RECEIVE_COMPLETE);

            return true;
        }

        void copyReceivedData(uint8_t* data, size_t size)
        {
            const auto count = std::min(size, receivedData.size());
            for(size_t i = 0; i < count; ++i)
            {
                data[i] = receivedData.front();
                receivedData.pop_front();
            }
            std::memset(data + count, 0, size - count);
        }

        void notifyInterrupt(Event event)
        {
            std::lock_guard<std::mutex> lock(interruptMutex);
            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::UART, event});
        }

        std::mutex stateMutex;
        std::mutex interruptMutex;
        InterconnectUart* peer = nullptr;
        bool initialized = false;
        uint8_t* pendingData = nullptr;
        size_t pendingSize = 0;
        std::deque<uint8_t> receivedData;
        npos::dev::hal::InterruptHandler* interruptHandler = nullptr;
    };
}
