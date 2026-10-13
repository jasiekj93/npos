#pragma once

/**
 * @file Console.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstring>

#include <etl/string.h>

#include <libnpos/dev/Api.hpp>

#include "RecordingApi.hpp"

namespace mock
{
    // Plays the uart device for a terminal: collects what it writes and feeds it typed keys.
    class Console
    {
    public:
        Console(npos::ipc::Service& terminal, RecordingApi& api)
            : terminal(terminal)
            , api(api)
        {
        }

        // Answers the device open request.
        void open(bool success = true)
        {
            auto message = api.sent.front();
            api.sent.erase(api.sent.begin());
            auto& response = reinterpret_cast<npos::dev::msg::Open&>(message);
            response.setStatus(success ? npos::dev::msg::Open::SUCCESS : npos::dev::msg::Open::INIT_FAILURE).setOpened({ 5, 0 });
            message.sender = api.portId;
            terminal.handle(message);
            pump();
        }

        // Handles pending requests: writes are captured and completed, reads wait for keys.
        void pump()
        {
            while(not api.sent.empty())
            {
                // The open request waits for open().
                if(api.sent.front().type == npos::dev::msg::Type::OPEN)
                    break;

                auto message = api.sent.front();
                api.sent.erase(api.sent.begin());

                if(message.type != npos::dev::msg::Type::TRANSCIEVER)
                    continue;

                auto& request = reinterpret_cast<npos::dev::msg::Transciever&>(message);
                message.sender = api.portId;

                if(request.getOperation() == npos::dev::msg::Transciever::WRITE)
                {
                    auto data = request.getInputBuffer();
                    output.append(reinterpret_cast<const char*>(data.data()), data.size());
                    writes++;

                    if(not holdWrites)
                    {
                        request.setStatus(npos::dev::msg::Transciever::SUCCESS).setOutputLength(data.size());
                        terminal.handle(message);
                    }
                    else
                        heldWrite = message;
                }
                else
                {
                    reading = true;
                    readRequest = message;
                }
            }
        }

        // Completes a write that was held back.
        void completeHeldWrite()
        {
            auto& request = reinterpret_cast<npos::dev::msg::Transciever&>(heldWrite);
            request.setStatus(npos::dev::msg::Transciever::SUCCESS).setOutputLength(request.getInputLength());
            holdWrites = false;
            terminal.handle(heldWrite);
            pump();
        }

        // Delivers keys in as few reads as the pending read buffer allows.
        void type(etl::string_view keys)
        {
            pump();

            while(not keys.empty() and reading)
            {
                auto size = keys.size() < readRequest.output.size ? keys.size() : readRequest.output.size;
                deliver(keys.substr(0, size));
                keys.remove_prefix(size);
            }
        }

        // Delivers bytes as one completed read, the rest of the buffer is zero filled.
        void deliver(etl::string_view bytes)
        {
            pump();

            if(not reading)
                return;

            reading = false;
            auto& request = reinterpret_cast<npos::dev::msg::Transciever&>(readRequest);
            std::memset(readRequest.output.data, 0, readRequest.output.size);
            std::memcpy(readRequest.output.data, bytes.data(), bytes.size());
            request.setStatus(npos::dev::msg::Transciever::SUCCESS).setOutputLength(readRequest.output.size);
            terminal.handle(readRequest);
            pump();
        }

        // Completes the pending read with the given status and no data.
        void failRead(npos::dev::msg::Transciever::Status status = npos::dev::msg::Transciever::DEVICE_ERROR)
        {
            pump();

            if(not reading)
                return;

            reading = false;
            auto& request = reinterpret_cast<npos::dev::msg::Transciever&>(readRequest);
            request.setStatus(status).setOutputLength(0);
            terminal.handle(readRequest);
            pump();
        }

        // Takes and clears the text written so far.
        etl::string<4096> take()
        {
            pump();
            auto result = output;
            output.clear();
            return result;
        }

        npos::ipc::Service& terminal;
        RecordingApi& api;
        etl::string<4096> output;
        bool reading = false;
        bool holdWrites = false;
        int writes = 0;
        npos::ipc::Message readRequest;
        npos::ipc::Message heldWrite;
    };
}
