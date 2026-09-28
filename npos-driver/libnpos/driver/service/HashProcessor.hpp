#pragma once

/**
 * @file HashProcessor.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <libnpos/device/Transciever.hpp>
#include <libnpos/driver/hal/Uart.hpp>
#include <libnpos/kernel/Service.hpp>

namespace npos::driver::service
{
    class UartInterrupt : public kernel::Service, public hal::InterruptHandler
    {
    public:
        explicit UartInterrupt(hal::UartInterrupt&, kernel::Bus&, device::TranscieverRequest::Id id);

        void initalize() override;
        bool accepts(kernel::Message::Type) const override;

        void handleInterrupt(const hal::Interrupt&) override;

    protected:
        void onReceive(kernel::Message&) override;

    private:
        hal::UartInterrupt& uart;
        device::TranscieverRequest::Id id;
        kernel::Message* transmitRequest;
        kernel::Message* receiveRequest;
    };
}