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
            hash = file = ipc::NULL_OID;
            result = msg::FileHash::Status::SUCCESS;
            openingDevice = false;
            lastChunk = false;

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
                    return fail(msg::FileHash::Status::IS_A_DIRECTORY);

                openingDevice = true;
                npos::dev::api::open(*this, hashName);
                npos::dev::api::timerRegister(*this, npos::dev::msg::Timer::Mode::SINGLE_SHOT, 1000);
                timers++;
            }
            else if(response.getStatus() == npos::fs::msg::Status::NO_DIR_ENTRY)
                fail(msg::FileHash::Status::FILE_NOT_FOUND);
            else
                fail(msg::FileHash::Status::DEVICE_ERROR);
        }

        void onTimer(npos::dev::msg::Timer& response)
        {
            // A timer cannot be called off, so the ones of the earlier requests also come. Only the last one counts.
            if(timers > 0)
                timers--;

            if(timers > 0 or not currentRequest.hasValue() or not openingDevice)
                return;

            openingDevice = false;

            if(response.getStatus() != npos::dev::msg::Timer::Status::SUCCESS)
                return fail(msg::FileHash::Status::DEVICE_ERROR);

            fail(msg::FileHash::Status::HASH_NOT_FOUND);
        }

        void onDeviceOpen(npos::dev::msg::Open& response)
        {
            if(not currentRequest.hasValue() or not openingDevice)
                return;

            openingDevice = false;

            if(response.getStatus() == npos::dev::msg::Open::Status::SUCCESS)
            {
                hash = response.getOpened();
                auto* request = currentRequest.get();
                npos::fs::api::file::open(*this, request->getPath(), npos::fs::Flags::READ_ONLY);
            }
            else if(response.getStatus() == npos::dev::msg::Open::Status::LOCKED)
                fail(msg::FileHash::Status::HASH_BUSY);
            else
                fail(msg::FileHash::Status::DEVICE_ERROR);
        }

        void onFileOpen(npos::fs::msg::file::Open& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(msg::FileHash::Status::DEVICE_ERROR);

            file = response.getOpened();
            remaining = response.getSize();

            readNext();
        }

        void onFileRead(npos::fs::msg::file::Read& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(msg::FileHash::Status::DEVICE_ERROR);

            // A file that got shorter in the meantime ends with what was read.
            chunkSize = etl::min(response.getOutputLength(), readSize);
            remaining -= chunkSize;
            lastChunk = (remaining == 0 or chunkSize == 0);

            auto data = etl::span<const uint8_t>(buffer.data(), chunkSize);

            if(lastChunk)
            {
                auto* request = const_cast<msg::FileHash*>(currentRequest.get());
                npos::dev::api::hashAccumulateEnd(*this, hash, data, request->getOutputBuffer().first(HASH_SIZE));
            }
            else
                npos::dev::api::hashAccumulate(*this, hash, data);
        }

        void onHash(npos::dev::msg::Hash& response)
        {
            if(not currentRequest.hasValue())
                return;

            if(response.getStatus() != npos::dev::msg::Hash::Status::SUCCESS)
                return fail(msg::FileHash::Status::DEVICE_ERROR);

            if(lastChunk)
                closeResources();
            else
                readNext();
        }

        void onFileClose(npos::fs::msg::file::Close& response)
        {
            if(not currentRequest.hasValue())
                return;

            file = ipc::NULL_OID;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                keepFailure(msg::FileHash::Status::DEVICE_ERROR);

            if(hash == ipc::NULL_OID)
                respond();
        }

        void onDeviceClose(npos::dev::msg::Close& response)
        {
            if(not currentRequest.hasValue())
                return;

            hash = ipc::NULL_OID;

            if(response.getStatus() != npos::dev::msg::Close::Status::SUCCESS)
                keepFailure(msg::FileHash::Status::DEVICE_ERROR);

            if(file == ipc::NULL_OID)
                respond();
        }

        void readNext()
        {
            readSize = etl::min(remaining, buffer.size());
            npos::fs::api::file::read(*this, file, readSize, buffer);
        }

        // Ends the request with the status, but only after the file and the device are closed.
        void fail(msg::FileHash::Status status)
        {
            result = status;
            closeResources();
        }

        // The closes are sent together, the request is answered when both of them are done.
        void closeResources()
        {
            if(file == ipc::NULL_OID and hash == ipc::NULL_OID)
                return respond();

            if(file != ipc::NULL_OID)
                npos::fs::api::file::close(*this, file);

            if(hash != ipc::NULL_OID)
                npos::dev::api::close(*this, hash);
        }

        void keepFailure(msg::FileHash::Status status)
        {
            if(result == msg::FileHash::Status::SUCCESS)
                result = status;
        }

        void respond()
        {
            if(auto* request = currentRequest.release())
                api.respond(request->setStatus(result));
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
        size_t chunkSize = 0;
        size_t timers = 0;
        bool openingDevice = false;
        bool lastChunk = false;
        msg::FileHash::Status result = msg::FileHash::Status::SUCCESS;
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
