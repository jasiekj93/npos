#pragma once

/**
 * @file UserApplication.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <iostream>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/Api.hpp>

class UserApplication : public npos::ipc::Service
{
public:
	UserApplication(Api& api, etl::string_view devName) 
        : npos::ipc::Service(api) 
        , devName(devName)
        {}

    void initialize() override
    {
        npos::dev::api::open(*this, devName);
        std::cout << "UserApplication initialized." << std::endl;
    }

    void handleRequest(npos::ipc::Message& message) override {}

    void handleResponse(npos::ipc::Message& message) override
    {
        if (message.type == npos::dev::msg::Type::OPEN) 
        {
            auto& response = static_cast<npos::dev::msg::Open&>(message);

            if(response.getStatus() == npos::dev::msg::Open::Status::SUCCESS)
            {
                std::cout << "Device opened." << std::endl;
                devOid = response.getOpened();

                std::cout << "Setting timer periodic for 1s" << std::endl;
                npos::dev::api::timerRegister(*this, npos::dev::msg::Timer::Mode::SINGLE_SHOT, 1000);

            }
        }
        else if (message.type == npos::dev::msg::Type::LED) 
        {
            auto& response = static_cast<npos::dev::msg::Led&>(message);

            if(response.getStatus() == npos::dev::msg::Led::Status::SUCCESS)
            {
                std::cout << "Led: " << response.getId() << " toggled." << std::endl;
            }
            else
            {
                std::cout << "Led: " << response.getId() << " failed to toggle." << std::endl;
            }
        }

        else if (message.type == npos::dev::msg::Type::TIMER) 
        {
            auto& response = static_cast<npos::dev::msg::Timer&>(message);

            if(response.getStatus() == npos::dev::msg::Timer::Status::NO_MEMORY)
            {
                std::cout << "Timer registration failed: No memory." << std::endl;
            }
            else
            {
                std::cout << "Toggling led0" << std::endl;
                npos::dev::api::ledToggle(*this, devOid, 0);

                std::cout << "Toggling led1" << std::endl;
                npos::dev::api::ledToggle(*this, devOid, 1); 
            }

        }
    }

    bool acceptsRequest(npos::ipc::Message::Type type) const override { return false; }

private:
    etl::string_view devName;
    npos::ipc::Oid devOid;
};