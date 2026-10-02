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

                std::cout << "Writing Uart" << std::endl;
                npos::dev::api::transcieverWrite(*this, devOid, { input, 1024 });
            }
        }
        else if (message.type == npos::dev::msg::Type::TRANSCIEVER) 
        {
            auto& response = static_cast<npos::dev::msg::Transciever&>(message);

            if(response.getStatus() == npos::dev::msg::Transciever::Status::SUCCESS)
            {
                if(response.getOperation() == npos::dev::msg::Transciever::Operation::WRITE)
                {
                    std::cout << "Uart write completed successfully." << std::endl;
                    std::cout << "Reading from Uart" << std::endl;
                    npos::dev::api::transcieverRead(*this, devOid, 1024, { output, 1024 });
                }
                else if(response.getOperation() == npos::dev::msg::Transciever::Operation::READ)
                {
                    std::cout << "Uart read completed successfully." << std::endl;
                }
            }
            else
            {
                std::cout << "Uart operation failed." << std::endl;
            }
        }
    }

    bool accepts(npos::ipc::Message::Type type) const override
    {
        return type == npos::dev::msg::Type::OPEN or
            type == npos::dev::msg::Type::TRANSCIEVER;
    }

private:
    etl::string_view devName;
    npos::ipc::Oid devOid;
    uint8_t input[1024];
    uint8_t output[1024];
};