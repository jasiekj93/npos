#pragma once

/**
 * @file Littlefs.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/os/filesystem/ServiceTask.hpp>
#include <libnpos/fs/service/Littlefs.hpp>

namespace npos::fs::task
{
    class Littlefs : public os::filesystem::ServiceTask
    {
    public:
        Littlefs(Priority priority, Tid tid,
            hal::Flash& flash, SystemBus& bus, hal::ICache* icache, 
            service::Littlefs::FileStorage& storage, 
            service::Littlefs::DirectoryStorage& directoryStorage,
            MessageQueue& messageQueue)
            : os::filesystem::ServiceTask(priority, tid, bus, service, messageQueue)
            , service(flash, bus, icache, storage, directoryStorage)
        {
        }

    private:
        service::Littlefs service;
    };
}