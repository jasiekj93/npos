#pragma once

/**
 * @file LedHandler.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/driver/message/Led.hpp>
#include <libnpos/os/driver/hal/Led.hpp>
#include <libnpos/os/Service.hpp>

namespace npos::os::driver::service
{
    class LedHandler : public os::Service
    {
    public:
        explicit LedHandler(hal::Led&, os::message::Bus&);

        void initalize() override;
        void onReceive(const ipc::Message&) override;
        bool accepts(ipc::Message::Id) const override;

    private:
        hal::Led& led;
    };
}