#pragma once

/**
 * @file Littlefs.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <liblittlefs/lfs.h>

#include <libnpos/os/Service.hpp>
#include <libnpos/fs/hal/Flash.hpp>


// #include <libnpos/fs/message/

namespace npos::fs::service
{
    class Littlefs : public npos::os::Service
    {
    public:
        Littlefs(hal::Flash& flash, os::message::Bus& bus);

        void initalize() override;

        void onReceive(const ipc::Message&) override;
        bool accepts(ipc::Message::Id) const override;

    private:
        lfs_t lfs;
        lfs_config config;
        hal::Flash& flash;
    };
}