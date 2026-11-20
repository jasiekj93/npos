#pragma once

/**
 * @file FileHash.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <etl/span.h>
#include <etl/string_view.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/ipc/MessageStore.hpp>

#include <libnpos/dev/Api.hpp>
#include <libnpos/fs/Api.hpp>
#include <libnpos/mdlwr/dev/Api.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>

namespace npos::mdlwr::dev::service
{
    class FileHash : public ipc::Service
    {
    public:
        static constexpr size_t HASH_SIZE = 32;

        FileHash(Api& api, etl::string_view hashName, etl::span<uint8_t> buffer)
            : ipc::Service(api)
            , hashName(hashName)
            , buffer(buffer) 
            , currentRequest(*this)
        {
        }

        void handleRequest(ipc::Message& message) override
        {
            auto& request = static_cast<npos::mdlwr::dev::msg::FileHash&>(message);

            if(currentRequest.hasValue())
                return api.respond(request.setStatus(msg::FileHash::Status::BUSY));

            if(request.getOutputBuffer().size() < HASH_SIZE)
                return api.respond(request.setStatus(msg::FileHash::Status::BUFFER_TOO_SMALL));

            currentRequest.store(request);

            npos::fs::api::stat(*this, request.getPath(), info);
        }

        void handleResponse(ipc::Message& message) override
        {
            switch(message.type)
            {
            case npos::fs::msg::Type::STAT:
                return onStat(static_cast<npos::fs::msg::Stat&>(message));
            case npos::dev::msg::Type::TIMER:
                return onTimer(static_cast<npos::dev::msg::Timer&>(message));
            case npos::dev::msg::Type::OPEN:
                return onDeviceOpen(static_cast<npos::dev::msg::Open&>(message));
            case npos::fs::msg::Type::FILE_OPEN:
                return onFileOpen(static_cast<npos::fs::msg::file::Open&>(message));
            case npos::fs::msg::Type::FILE_READ:
                return onFileRead(static_cast<npos::fs::msg::file::Read&>(message));
            case npos::dev::msg::Type::HASH:
                return onHash(static_cast<npos::dev::msg::Hash&>(message));
            case npos::fs::msg::Type::FILE_CLOSE:
                return onFileClose(static_cast<npos::fs::msg::file::Close&>(message));
            case npos::dev::msg::Type::CLOSE:
                return onDeviceClose(static_cast<npos::dev::msg::Close&>(message));
            }
        }

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return type == msg::Type::FILE_HASH;
        }

    protected:
        void onStat(npos::fs::msg::Stat& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                if(info.type == npos::fs::Info::Type::DIRECTORY)
                {
                    auto* request = currentRequest.release();
                    return api.respond(request->setStatus(msg::FileHash::Status::IS_A_DIRECTORY));
                }

                npos::dev::api::open(*this, hashName);
                npos::dev::api::timerRegister(*this, npos::dev::msg::Timer::Mode::SINGLE_SHOT, 1000);
            }
            else if(response.getStatus() == npos::fs::msg::Status::NO_DIR_ENTRY)
            {
                auto* request = currentRequest.release();
                api.respond(request->setStatus(msg::FileHash::Status::FILE_NOT_FOUND));
            }
            else
            {
                auto* request = currentRequest.release();
                api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }
        }

        void onTimer(npos::dev::msg::Timer& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::dev::msg::Timer::Status::SUCCESS)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }

            if(hash == ipc::NULL_OID)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::HASH_NOT_FOUND));
            }
        }

        void onDeviceOpen(npos::dev::msg::Open& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() == npos::dev::msg::Open::Status::SUCCESS)
            {
                hash = response.getOpened();
                auto* request = currentRequest.get();
                npos::fs::api::file::open(*this, request->getPath(), npos::fs::Flags::READ_ONLY);
            }
            else if(response.getStatus() == npos::dev::msg::Open::Status::LOCKED)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::HASH_BUSY));
            }
            else
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }
        }

        void onFileOpen(npos::fs::msg::file::Open& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }

            file = response.getOpened();
            remaining = response.getSize();

            readSize = etl::min(remaining, buffer.size());
            npos::fs::api::file::read(*this, file, readSize, buffer);
        }

        void onFileRead(npos::fs::msg::file::Read& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }

            auto* request = const_cast<msg::FileHash*>(currentRequest.get());

            if(readSize < buffer.size())
                npos::dev::api::hashAccumulateEnd(*this, hash, etl::span<const uint8_t>(buffer.data(), readSize), request->getOutputBuffer());
            else
                npos::dev::api::hashAccumulate(*this, hash, etl::span<const uint8_t>(buffer.data(), readSize));
        }

        void onHash(npos::dev::msg::Hash& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::dev::msg::Hash::Status::SUCCESS)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }

            remaining -= readSize;

            if(remaining == 0)
            {
                npos::fs::api::file::close(*this, file);
                npos::dev::api::close(*this, hash);
            }
            else
            {
                readSize = etl::min(remaining, buffer.size());
                npos::fs::api::file::read(*this, file, readSize, buffer);
            }
        }

        void onFileClose(npos::fs::msg::file::Close& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }

            file = ipc::NULL_OID;

            if(hash == ipc::NULL_OID)
            {
                auto* request = currentRequest.release();
                api.respond(request->setStatus(msg::FileHash::Status::SUCCESS));
            }
        }

        void onDeviceClose(npos::dev::msg::Close& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::dev::msg::Close::Status::SUCCESS)
            {
                auto* request = currentRequest.release();
                return api.respond(request->setStatus(msg::FileHash::Status::DEVICE_ERROR));
            }

            hash = ipc::NULL_OID;

            if(file == ipc::NULL_OID)
            {
                auto* request = currentRequest.release();
                api.respond(request->setStatus(msg::FileHash::Status::SUCCESS));
            }
        }

    private:
        etl::string_view hashName;
        etl::span<uint8_t> buffer;
        ipc::Oid hash = ipc::NULL_OID;
        ipc::MessageStore<msg::FileHash> currentRequest;
        npos::fs::Info info;
        size_t remaining = 0;
        ipc::Oid file = ipc::NULL_OID;
        size_t readSize = 0;
    };

    template<size_t BUFFER_SIZE>
    class FileHashStatic : public FileHash
    {
    public:
        FileHashStatic(Api& api, etl::string_view name)
            : FileHash(api, name, staticBuffer)
        {
        }

    private:
        etl::array<uint8_t, BUFFER_SIZE> staticBuffer;
    };
}
