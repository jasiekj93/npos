#pragma once

/**
 * @file ProgramHarness.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstring>

#include <libnpos/fs/Api.hpp>

#include "FakeHost.hpp"
#include "RecordingApi.hpp"

namespace mock
{
    // Plays the filesystem side for a program: takes its requests from the api and answers them.
    class ProgramHarness
    {
    public:
        explicit ProgramHarness(npos::cli::Program& program, RecordingApi& api)
            : program(program)
            , api(api)
        {
        }

        bool hasRequest() const { return not api.sent.empty(); }

        // Pops the oldest request the program sent.
        npos::ipc::Message take()
        {
            auto message = api.sent.front();
            api.sent.erase(api.sent.begin());
            return message;
        }

        npos::ipc::Message::Type nextType() const { return api.sent.front().type; }

        void answerOpen(npos::fs::msg::Status status, npos::ipc::Oid opened = { 5, 0 })
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Open&>(message);
            response.setStatus(status).setOpened(opened);
            program.handle(message);
        }

        void answerDirOpen(npos::fs::msg::Status status, npos::ipc::Oid opened = { 5, 0 })
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::dir::Open&>(message);
            response.setStatus(status).setOpened(opened);
            program.handle(message);
        }

        void answerClose(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Close&>(message);
            response.setStatus(status);
            program.handle(message);
        }

        void answerDirClose(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::dir::Close&>(message);
            response.setStatus(status);
            program.handle(message);
        }

        void answerRead(const char* data, npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Read&>(message);
            auto length = std::strlen(data);
            std::memcpy(message.output.data, data, length);
            response.setOutputLength(length).setStatus(status);
            program.handle(message);
        }

        void answerWrite(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Write&>(message);
            response.setOutputLength(message.input.size).setStatus(status);
            program.handle(message);
        }

        void answerDirRead(npos::fs::msg::Status status, npos::fs::msg::dir::Entry::Type type = npos::fs::msg::dir::Entry::FILE,
            uint32_t size = 0, const char* name = "")
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::dir::Read&>(message);
            auto& entry = response.getEntry();
            entry.type = type;
            entry.size = size;
            std::strncpy(entry.name, name, sizeof(entry.name) - 1);
            response.setStatus(status);
            program.handle(message);
        }

        npos::cli::Program& program;
        RecordingApi& api;
    };
}
