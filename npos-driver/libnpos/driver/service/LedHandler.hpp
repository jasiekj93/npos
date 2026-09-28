#pragma once

/**
 * @file LedHandler.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/device/Led.hpp>
#include <libnpos/driver/hal/Led.hpp>
#include <libnpos/kernel/Service.hpp>

namespace npos::driver::service
{
    class LedHandler : public kernel::Service
    {
    public:
        explicit LedHandler(hal::Led&, kernel::Bus&);

        bool accepts(kernel::Message::Type) const override;

    protected:
        void onReceive(kernel::Message&) override;

    private:
        hal::Led& led;
    };
}