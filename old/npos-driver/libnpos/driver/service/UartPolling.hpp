#pragma once

/**
 * @file UartPolling.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <libnpos/device/Transciever.hpp>
#include <libnpos/driver/hal/Uart.hpp>
#include <libnpos/kernel/Service.hpp>

namespace npos::driver::service
{
    class UartPolling : public kernel::Service
    {
    public:
        explicit UartPolling(hal::UartPolling&, kernel::Bus&, device::TranscieverRequest::Id id);

        void initalize() override;
        bool accepts(kernel::Message::Type) const override;

    protected:
        void onReceive(kernel::Message&) override;

    private:
        hal::UartPolling& uart;
        device::TranscieverRequest::Id id;
    };
}