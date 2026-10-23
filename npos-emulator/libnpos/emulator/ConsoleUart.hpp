#pragma once

/**
 * @file ConsoleUart.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <condition_variable>
#include <mutex>
#include <system_error>
#include <thread>
#include <atomic>

#include <poll.h>
#include <unistd.h>

#include <libnpos/dev/hal/Uart.hpp>

namespace npos::emulator
{
    // Uart backed by stdin/stdout; input is received asynchronously on a reader thread.
    class ConsoleUart : public npos::dev::hal::UartInterrupt
    {
    public:
        ~ConsoleUart() override
        {
            deinit();
        }

        bool init() override
        {
            if(initialized)
                return true;

            stopReader = false;
            inputEnded = false;
            initialized = true;

            try
            {
                reader = std::thread(&ConsoleUart::readInput, this);
            }
            catch(const std::system_error&)
            {
                initialized = false;
                return false;
            }

            return true;
        }

        void deinit() override
        {
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                initialized = false;
                pendingData = nullptr;
                pendingSize = 0;
                stopReader = true;
            }

            stateChanged.notify_all();

            if(reader.joinable())
                reader.join();
        }

        bool transmitIt(const uint8_t* data, size_t size) override 
        {
            if(not initialized)
                return false;

            std::fwrite(data, 1, size, stdout);
            std::fflush(stdout);

            notifyInterrupt(Event::TRANSMIT_COMPLETE);
            return true;
        }

        bool receiveIt(uint8_t* data, size_t size) override
        {
            std::lock_guard<std::mutex> lock(stateMutex);

            if(not initialized or inputEnded or pendingData or size == 0)
                return false;

            pendingData = data;
            pendingSize = size;
            stateChanged.notify_one();
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

        bool initialized = false;
        npos::dev::hal::InterruptHandler* interruptHandler = nullptr;

    private:
        void readInput()
        {
            pollfd input{STDIN_FILENO, POLLIN, 0};

            while(not stopReader)
            {
                {
                    std::unique_lock<std::mutex> lock(stateMutex);
                    stateChanged.wait(lock, [this] { return stopReader or pendingData; });
                }

                if(stopReader)
                    break;

                auto result = ::poll(&input, 1, 50);

                if(stopReader)
                    break;

                if(result == 0)
                    continue;

                if(result < 0)
                {
                    if(errno == EINTR)
                        continue;

                    finishPending(Event::ERROR, true);
                    break;
                }

                if(not (input.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)))
                    continue;

                Event event = Event::ERROR;

                {
                    std::lock_guard<std::mutex> lock(stateMutex);

                    if(not pendingData)
                        continue;

                    if(input.revents & POLLNVAL)
                    {
                        inputEnded = true;
                        pendingData = nullptr;
                        pendingSize = 0;
                    }
                    else
                    {
                        auto received = ::read(STDIN_FILENO, pendingData, pendingSize);

                        if(received > 0)
                        {
                            std::memset(pendingData + received, 0, pendingSize - static_cast<size_t>(received));
                            event = Event::RECEIVE_COMPLETE;
                        }
                        else if(received == 0)
                            inputEnded = true;
                        else if(errno == EINTR or errno == EAGAIN or errno == EWOULDBLOCK)
                            continue;
                        else
                            inputEnded = true;

                        pendingData = nullptr;
                        pendingSize = 0;
                    }
                }

                notifyInterrupt(event);
            }
        }

        void finishPending(Event event, bool endInput)
        {
            bool hasPending = false;

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                inputEnded = endInput;
                hasPending = pendingData != nullptr;
                pendingData = nullptr;
                pendingSize = 0;
            }

            if(hasPending)
                notifyInterrupt(event);
        }

        void notifyInterrupt(Event event)
        {
            std::lock_guard<std::mutex> lock(interruptMutex);

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::UART, event});
        }

        std::mutex stateMutex;
        std::condition_variable stateChanged;
        std::mutex interruptMutex;
        std::thread reader;
        std::atomic<bool> stopReader{false};
        bool inputEnded = false;
        uint8_t* pendingData = nullptr;
        size_t pendingSize = 0;
    };
}
