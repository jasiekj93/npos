#pragma once

/**
 * @file Filesystem.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <algorithm>
#include <cstring>

#include <libnpos/fs/Api.hpp>
#include <libnpos/mdlwr/fs/msg/file/Copy.hpp>

#include "mock/RecordingApi.hpp"

namespace mock
{
    // Plays the filesystem side for a service: takes its requests from the api and answers them.
    class Filesystem
    {
    public:
        Filesystem(npos::ipc::Service& service, RecordingApi& api)
            : service(service)
            , api(api)
        {
        }

        bool hasRequest() const { return not api.sent.empty(); }

        // Pops the oldest request the service sent.
        npos::ipc::Message take()
        {
            auto message = api.sent.front();
            api.sent.erase(api.sent.begin());
            return message;
        }

        npos::ipc::Message::Type nextType() const { return api.sent.front().type; }

        npos::ipc::Oid nextRecipient() const { return api.sent.front().recipient; }

        // The path of the oldest request, which carries it in the input.
        const char* nextPath() const { return reinterpret_cast<const char*>(api.sent.front().input.data); }

        void answerStat(npos::fs::msg::Status status, npos::fs::Info::Type type = npos::fs::Info::FILE)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::Stat&>(message);
            response.getEntry().type = type;
            response.setStatus(status);
            service.handleResponse(message);
        }

        void answerOpen(npos::fs::msg::Status status, size_t size = 0, npos::ipc::Oid opened = { 5, 0 })
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Open&>(message);
            response.setStatus(status).setOpened(opened).setSize(size);
            service.handleResponse(message);
        }

        void answerClose(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Close&>(message);
            response.setStatus(status);
            service.handleResponse(message);
        }

        // Fills the buffer of the request with the data, but not with more than the buffer takes.
        void answerRead(const char* data, npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Read&>(message);
            auto length = std::min(std::strlen(data), message.output.size);
            std::memcpy(message.output.data, data, length);
            response.setOutputLength(length).setStatus(status);
            service.handleResponse(message);
        }

        void answerWrite(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS, size_t length = SIZE_MAX)
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::file::Write&>(message);
            response.setOutputLength(std::min(length, message.input.size)).setStatus(status);
            service.handleResponse(message);
        }

        void answerFileRemove(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            reinterpret_cast<npos::fs::msg::file::Remove&>(message).setStatus(status);
            service.handleResponse(message);
        }

        void answerRename(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            reinterpret_cast<npos::fs::msg::Rename&>(message).setStatus(status);
            service.handleResponse(message);
        }

        void answerDirOpen(npos::fs::msg::Status status, npos::ipc::Oid opened = { 5, 1 })
        {
            auto message = take();
            reinterpret_cast<npos::fs::msg::dir::Open&>(message).setStatus(status).setOpened(opened);
            service.handleResponse(message);
        }

        void answerDirRead(npos::fs::msg::Status status, npos::fs::Info::Type type = npos::fs::Info::FILE,
            const char* name = "")
        {
            auto message = take();
            auto& response = reinterpret_cast<npos::fs::msg::dir::Read&>(message);
            response.getEntry().type = type;
            std::strncpy(response.getEntry().name, name, sizeof(response.getEntry().name) - 1);
            response.setStatus(status);
            service.handleResponse(message);
        }

        void answerEndOfDirectory()
        {
            answerDirRead(npos::fs::msg::Status::END_OF_DIRECTORY);
        }

        void answerDirClose(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            reinterpret_cast<npos::fs::msg::dir::Close&>(message).setStatus(status);
            service.handleResponse(message);
        }

        void answerDirMake(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            reinterpret_cast<npos::fs::msg::dir::Make&>(message).setStatus(status);
            service.handleResponse(message);
        }

        void answerDirRemove(npos::fs::msg::Status status = npos::fs::msg::Status::SUCCESS)
        {
            auto message = take();
            reinterpret_cast<npos::fs::msg::dir::Remove&>(message).setStatus(status);
            service.handleResponse(message);
        }

        // The path a request carrying two of them (copy, move) keeps in the output.
        const char* nextNewPath() const { return reinterpret_cast<const char*>(api.sent.front().output.data); }

        // Answers a request of the middleware, e.g. answerMiddleware<npos::mdlwr::fs::msg::dir::Move>(status).
        template<typename Request>
        void answerMiddleware(npos::mdlwr::fs::msg::Status status)
        {
            auto message = take();
            reinterpret_cast<Request&>(message).setStatus(status);
            service.handleResponse(message);
        }

        // The middleware request a directory service sends to the file service.
        void answerFileCopy(npos::mdlwr::fs::msg::Status status)
        {
            auto message = take();
            reinterpret_cast<npos::mdlwr::fs::msg::file::Copy&>(message).setStatus(status);
            service.handleResponse(message);
        }

        npos::ipc::Service& service;
        RecordingApi& api;
    };
}
