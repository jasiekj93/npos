#pragma once

/**
 * @file Littlefs.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/queue.h>
#include <etl/array.h>
#include <etl/string.h>

#include <cstring>

#include <liblittlefs/lfs.h>

#include <libnpos/nps/Task.hpp>
#include <libnpos/ipc/Port.hpp>
#include <libnpos/ipc/Bus.hpp>

#include <libnpos/fs/Api.hpp>
#include <libnpos/fs/hal/Flash.hpp>
#include <libnpos/fs/hal/ICache.hpp>

namespace npos::fs::task
{
    template<size_t N>
    class Littlefs : public npos::nps::Task, public ipc::Port
    {
    public:
        static constexpr size_t READ_SIZE = 256;

        struct File
        {
            lfs_file_t handle;
            bool free;
        };

        struct Dir
        {
            lfs_dir_t handle;
            bool free;
        };

        using MessageQueueInt = etl::iqueue<ipc::Message*>;

        template<size_t M>
        using MessageQueue = etl::queue<ipc::Message*, M>;

        Littlefs(Priority priority, MessageQueueInt& messageQueue, ipc::Bus& bus, hal::Flash& flash, hal::ICache* icache)
            : nps::Task(priority)
            , messageQueue(messageQueue)
            , bus(bus)
            , flash(flash)
            , icache(icache)
        {
        }

        //Port
        bool accepts(ipc::Message::Type type) const override
        {
            switch (type)
            {
            case msg::Type::FILE_OPEN:
            case msg::Type::FILE_CLOSE:
            case msg::Type::FILE_READ:
            case msg::Type::FILE_WRITE:
            case msg::Type::FILE_REMOVE:
            case msg::Type::DIR_OPEN:
            case msg::Type::DIR_CLOSE:
            case msg::Type::DIR_READ:
            case msg::Type::DIR_MAKE:
            case msg::Type::DIR_REMOVE:
                return true;
            default:
                return false;
            }
        }

        void receiveRequest(const ipc::Message& message) override
        {
            if(messageQueue.full())
                return bus.release(const_cast<ipc::Message*>(&message));

            messageQueue.push(const_cast<ipc::Message*>(&message));
        }

        // The filesystem sends no requests, so there is nothing to answer.
        void receiveResponse(const ipc::Message& message) override
        {
            bus.release(const_cast<ipc::Message*>(&message));
        }

        void handleUnknownRequest(ipc::Message& message) override
        {
            message.output.status = ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED;
            bus.respond(message);
        }

        //nps::Task
        virtual void initialize() override
        {
            for (auto& file : files)
                file.free = true;

            for (auto& dir : dirs)
                dir.free = true;

            config.read = [](const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) -> int
            {
                auto fs = reinterpret_cast<Littlefs*>(c->context);
                return fs->readDisk(block, off, buffer,size);
            };

            config.prog = [](const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) -> int
            {
                auto fs = reinterpret_cast<Littlefs*>(c->context);
                return fs->programDisk(block, off, buffer, size);
            };

            config.erase = [](const struct lfs_config *c, lfs_block_t block) -> int
            {
                auto fs = reinterpret_cast<Littlefs*>(c->context);
                return fs->eraseDisk(block);
            };

            config.sync = [](const struct lfs_config *c) -> int
            {
                auto fs = reinterpret_cast<Littlefs*>(c->context);
                return fs->syncDisk();
            };

            config.read_size = READ_SIZE;
            config.prog_size = flash.getProgramSize();
            config.block_size = flash.getBlockSize();
            config.block_count = flash.getBlockCount();
            config.cache_size = READ_SIZE;
            config.lookahead_size = READ_SIZE;
            config.block_cycles = flash.getBlockCycles();
            config.context = this;

            if(icache != nullptr)
                icache->init();

            mounted = lfs_mount(&filesystem, &config) >= 0;

            if(not mounted and lfs_format(&filesystem, &config) >= 0)
                mounted = lfs_mount(&filesystem, &config) >= 0;
        }
        

        virtual bool isReady() const override
        {
            return not messageQueue.empty();
        }

        virtual void process()
        {
            if(messageQueue.empty())
                return; 

            auto message = messageQueue.front();
            handle(*message);
            messageQueue.pop();
            bus.release(message);
        }

        //Littlefs disk operations
        int readDisk(lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
        {
            size_t address = flash.getStartAddress() + (block * flash.getBlockSize()) + off;
            flash.read(address, static_cast<uint8_t*>(buffer), size);
            return LFS_ERR_OK;
        }

        int programDisk(lfs_block_t block, lfs_off_t off, const void* buff, lfs_size_t size)
        {
            size_t address = flash.getStartAddress() + (block * flash.getBlockSize()) + off;
            auto buffer = reinterpret_cast<const uint8_t*>(buff);

            if(icache != nullptr)
                if(not icache->disable())
                    return LFS_ERR_IO;

            if(not flash.unlock())
                return LFS_ERR_IO;

            for(size_t i = 0; i < size; i += flash.getProgramSize())
            {
                if(not flash.waitForLastOperation(0x10000))
                {
                    flash.lock();
                    return LFS_ERR_IO;
                }

                if(not flash.program(address, buffer))
                {
                    flash.lock();
                    return LFS_ERR_IO;
                }

                address += flash.getProgramSize();
                buffer += flash.getProgramSize();
            }

            if(not flash.lock())
                return LFS_ERR_IO;

            if(icache != nullptr)
                if(not icache->enable())
                    return LFS_ERR_IO;

            return LFS_ERR_OK;
        }

        int eraseDisk(lfs_block_t block)
        {
            if (not flash.unlock())
                return LFS_ERR_IO;

            if(not flash.eraseBlocks(block, 1))
            {
                flash.lock();
                return LFS_ERR_IO;
            }

            if(not flash.lock())
                return LFS_ERR_IO;

            return LFS_ERR_OK;
        }

        int syncDisk()
        {
            if(not flash.waitForLastOperation(0x10000))
            {
                flash.lock();
                return LFS_ERR_IO;
            }
            return LFS_ERR_OK;
        }

    protected:
        void handle(ipc::Message& message)
        {
            if(not mounted and accepts(message.type))
            {
                message.output.status = static_cast<ipc::Message::Status>(msg::Status::IO_ERROR);
                return bus.respond(message);
            }

            switch(message.type)
            {
            case msg::Type::FILE_OPEN:
                return fileOpen(static_cast<msg::file::Open&>(message));
            case msg::Type::FILE_CLOSE:
                return fileClose(static_cast<msg::file::Close&>(message));
            case msg::Type::FILE_READ:
                return fileRead(static_cast<msg::file::Read&>(message));
            case msg::Type::FILE_WRITE:
                return fileWrite(static_cast<msg::file::Write&>(message));
            case msg::Type::DIR_OPEN:
                return dirOpen(static_cast<msg::dir::Open&>(message));
            case msg::Type::DIR_CLOSE:
                return dirClose(static_cast<msg::dir::Close&>(message));
            case msg::Type::DIR_READ:
                return dirRead(static_cast<msg::dir::Read&>(message));
            case msg::Type::DIR_MAKE:
                return dirMake(static_cast<msg::dir::Make&>(message));
            case msg::Type::FILE_REMOVE:
                return fileRemove(static_cast<msg::file::Remove&>(message));
            case msg::Type::DIR_REMOVE:
                return dirRemove(static_cast<msg::dir::Remove&>(message));
            default:
                return handleUnknownRequest(message);
            }
        }

        void fileOpen(msg::file::Open& request)
        {
            for(auto i = 0; i < files.size(); i++)
            {
                if(files[i].free)
                {
                    auto result = lfs_file_open(&filesystem, &files[i].handle, 
                        request.getPath().data(), static_cast<int>(request.getFlags()));

                    if(result == LFS_ERR_OK)
                    {
                        files[i].free = false;
                        request.setOpened(ipc::Oid{ getId(), static_cast<ipc::ServiceId>(i) });
                        auto size = lfs_file_size(&filesystem, &files[i].handle);
                        request.setSize(size < 0 ? 0 : static_cast<size_t>(size));
                    }

                    return bus.respond(request.setStatus(fromLfsStatus(result)));
                }
            }

            bus.respond(request.setStatus(msg::Status::NO_MEMORY));
        }

        void dirOpen(msg::dir::Open& request)
        {
            for(size_t i = 0; i < dirs.size(); i++)
            {
                if(dirs[i].free)
                {
                    auto result = lfs_dir_open(&filesystem, &dirs[i].handle, request.getPath().data());

                    if(result == LFS_ERR_OK)
                    {
                        dirs[i].free = false;
                        request.setOpened(ipc::Oid{ getId(), static_cast<ipc::ServiceId>(i) });
                    }

                    return bus.respond(request.setStatus(fromLfsStatus(result)));
                }
            }

            bus.respond(request.setStatus(msg::Status::NO_MEMORY));
        }

        void dirMake(msg::dir::Make& request)
        {
            bus.respond(request.setStatus(fromLfsStatus(lfs_mkdir(&filesystem, request.getPath().data()))));
        }

        void dirRemove(msg::dir::Remove& request)
        {
            bus.respond(request.setStatus(remove(request.getPath().data(), LFS_TYPE_DIR)));
        }

        void fileRemove(msg::file::Remove& request)
        {
            bus.respond(request.setStatus(remove(request.getPath().data(), LFS_TYPE_REG)));
        }

        // lfs_remove accepts both types, so the type is checked first. The path must be null terminated.
        msg::Status remove(const char* path, uint8_t expectedType)
        {
            lfs_info info;
            auto result = lfs_stat(&filesystem, path, &info);

            if(result == LFS_ERR_OK)
            {
                if(info.type != expectedType)
                    return expectedType == LFS_TYPE_DIR ? msg::Status::NOT_A_DIRECTORY : msg::Status::IS_A_DIRECTORY;

                result = lfs_remove(&filesystem, path);
            }

            return fromLfsStatus(result);
        }

        void dirClose(msg::dir::Close& request)
        {
            if(request.getDir().serviceId >= dirs.size() or
                dirs[request.getDir().serviceId].free)
            {
                return bus.respond(request.setStatus(msg::Status::BAD_FILE_NUMBER));
            }

            auto& dir = dirs[request.getDir().serviceId];
            auto result = lfs_dir_close(&filesystem, &dir.handle);

            if(result == LFS_ERR_OK)
                dir.free = true;

            bus.respond(request.setStatus(fromLfsStatus(result)));
        }

        void dirRead(msg::dir::Read& request)
        {
            if(request.getDir().serviceId >= dirs.size() or
                dirs[request.getDir().serviceId].free)
            {
                return bus.respond(request.setStatus(msg::Status::BAD_FILE_NUMBER));
            }

            auto& dir = dirs[request.getDir().serviceId];
            auto& entry = request.getEntry();
            lfs_info info;
            int result;

            // "." and ".." are not reported.
            do
            {
                result = lfs_dir_read(&filesystem, &dir.handle, &info);
            }
            while(result > 0 and (strcmp(info.name, ".") == 0 or strcmp(info.name, "..") == 0));

            if(result < 0)
                return bus.respond(request.setStatus(fromLfsStatus(result)));

            if(result == 0)
                return bus.respond(request.setStatus(msg::Status::END_OF_DIRECTORY));

            entry.type = (info.type == LFS_TYPE_DIR) ? msg::dir::Entry::DIRECTORY : msg::dir::Entry::FILE;
            entry.size = info.size;
            strncpy(entry.name, info.name, sizeof(entry.name) - 1);
            entry.name[sizeof(entry.name) - 1] = '\0';

            bus.respond(request.setStatus(msg::Status::SUCCESS));
        }

        void fileClose(msg::file::Close& request)
        {
            if(request.getFile().serviceId >= files.size() or
                files[request.getFile().serviceId].free)
            {
                return bus.respond(request.setStatus(msg::Status::BAD_FILE_NUMBER));
            }

            auto& file = files[request.getFile().serviceId];
            auto result = lfs_file_close(&filesystem, &file.handle);

            if(result == LFS_ERR_OK)
                file.free = true;

            bus.respond(request.setStatus(fromLfsStatus(result)));
        }

        void fileRead(msg::file::Read& request)
        {
            if(request.getFile().serviceId >= files.size() or
                files[request.getFile().serviceId].free)
            {
                return bus.respond(request.setStatus(msg::Status::BAD_FILE_NUMBER));
            }

            auto& file = files[request.getFile().serviceId];
            auto result = lfs_file_read(&filesystem, &file.handle, request.getBuffer().data(), request.getBuffer().size());

            if(result >= 0)
                bus.respond(request.setOutputLength(static_cast<ipc::Message::Input::ReadWrite::Length>(result)).setStatus(msg::Status::SUCCESS));
            else
                bus.respond(request.setStatus(fromLfsStatus(result)));

        }

        void fileWrite(msg::file::Write& request)
        {
            if(request.getFile().serviceId >= files.size() or
                files[request.getFile().serviceId].free)
            {
                return bus.respond(request.setStatus(msg::Status::BAD_FILE_NUMBER));
            }

            auto& file = files[request.getFile().serviceId];
            auto result = lfs_file_write(&filesystem, &file.handle, request.getBuffer().data(), request.getBuffer().size());

            if(result >= 0)
                bus.respond(request.setOutputLength(static_cast<ipc::Message::Input::ReadWrite::Length>(result)).setStatus(msg::Status::SUCCESS));
            else
                bus.respond(request.setStatus(fromLfsStatus(result)));
        }

        msg::Status fromLfsStatus(int status)
        {
            switch(status)
            {
                case LFS_ERR_OK: return msg::Status::SUCCESS;
                case LFS_ERR_IO: return msg::Status::IO_ERROR;
                case LFS_ERR_CORRUPT: return msg::Status::CORRUPTED;
                case LFS_ERR_NOENT: return msg::Status::NO_DIR_ENTRY;
                case LFS_ERR_EXIST: return msg::Status::ENTRY_EXISTS;
                case LFS_ERR_NOTDIR: return msg::Status::NOT_A_DIRECTORY;
                case LFS_ERR_ISDIR: return msg::Status::IS_A_DIRECTORY;
                case LFS_ERR_NOTEMPTY: return msg::Status::DIRECTORY_NOT_EMPTY;
                case LFS_ERR_BADF: return msg::Status::BAD_FILE_NUMBER;
                case LFS_ERR_FBIG: return msg::Status::FILE_TOO_LARGE;
                case LFS_ERR_INVAL: return msg::Status::INVALID_PARAMETER;
                case LFS_ERR_NOSPC: return msg::Status::NO_SPACE_LEFT;
                case LFS_ERR_NOMEM: return msg::Status::NO_MEMORY;
                case LFS_ERR_NOATTR: return msg::Status::NO_ATTRIBUTE;
                case LFS_ERR_NAMETOOLONG: return msg::Status::NAME_TOO_LONG;
                default: return msg::Status::UNKNOWN_ERROR;
            }
        }

    private:
        MessageQueueInt& messageQueue;
        ipc::Bus& bus;
        hal::Flash& flash;
        hal::ICache* icache;
        etl::array<File, N> files;
        etl::array<Dir, N> dirs;
        lfs_t filesystem{};
        lfs_config config{};
        bool mounted = false;
    };

    template<size_t MESSAGE_SIZE, size_t FILES>
    class LittlefsStatic : public Littlefs<FILES>
    {
    public:
        using Base = Littlefs<FILES>;

        LittlefsStatic(npos::nps::Task::Priority priority, ipc::Bus& bus, hal::Flash& flash, hal::ICache* icache) 
            : Base(priority, messageQueueStatic, bus, flash, icache)
        {
        }

    private:
        typename Base::template MessageQueue<MESSAGE_SIZE> messageQueueStatic;
    };
}