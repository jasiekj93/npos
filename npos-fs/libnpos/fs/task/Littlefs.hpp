#pragma once

/**
 * @file Littlefs.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/queue.h>
#include <etl/array.h>

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
        struct File
        {
            lfs_file_t handle;
            bool free;
        };

        using MessageQueueInt = etl::iqueue<ipc::Message*>;

        template<size_t N>
        using MessageQueue = etl::queue<ipc::Message*, N>;

        Littlefs(Priority priority, MessageQueueInt& messageQueue, Bus& bus)
            : nps::Task(priority)
            , messageQueue(messageQueue)
            , bus(bus)
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
                return true;
            default:
                return false;
            }
        }

        void receive(const ipc::Message& message) override
        {
            if(messageQueue.full())
                return; //TODO handle
            else
                messageQueue.push(const_cast<Message*>(&message));
        }

        //nps::Task
        virtual void initialize() override
        {
            for (auto& file : files)
                file.free = true;
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

    protected:
        void handle(ipc::Message& message)
        {
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
            default:
                return;
            }
        }

        void fileOpen(msg::file::Open& request)
        {
            
        }

        void fileClose(msg::file::Open& request)
        {

        }

        void fileRead(msg::file::Open& request)
        {

        }

        void fileWrite(msg::file::Open& request)
        {

        }

    private:
        MessageQueueInt& messageQueue;
        ipc::Bus& bus;
        etl::array<File, N> files;
    };
}