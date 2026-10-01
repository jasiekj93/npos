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

    void handle(npos::ipc::Message& message) override
    {
        if (message.type == npos::dev::msg::Type::OPEN) 
        {
            auto& response = static_cast<npos::dev::msg::Open&>(message);

            if(response.getStatus() == npos::dev::msg::Open::Status::SUCCESS)
            {
                std::cout << "Device opened." << std::endl;
                devOid = response.getOpened();

                std::cout << "Getting time from device." << std::endl;
                npos::dev::api::getTime(*this, devOid);
            }
        }
        else if (message.type == npos::dev::msg::Type::CLOCK) 
        {
            auto& response = static_cast<npos::dev::msg::Clock&>(message);

            if(response.getStatus() == npos::dev::msg::Clock::Status::SUCCESS)
            {
                auto timestamp = response.getOutputTimestamp();
                std::cout << "Current time: " << timestamp << std::endl;
            }
        }
    }

    bool accepts(npos::ipc::Message::Type type) const override
    {
        return type == npos::dev::msg::Type::OPEN or
            type == npos::dev::msg::Type::CLOCK;
    }

private:
    etl::string_view devName;
    npos::ipc::Oid devOid;
};