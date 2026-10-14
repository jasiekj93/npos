#pragma once

/**
 * @file UserApplication.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <iostream>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/fs/Api.hpp>

class UserApplication : public npos::ipc::Service
{
public:
	UserApplication(Api& api) 
        : npos::ipc::Service(api) 
        {}

    void initialize() override
    {
        npos::fs::api::file::open(*this, "hello.txt", npos::fs::Flags::WRITE_ONLY | npos::fs::Flags::CREATE );
        std::cout << "UserApplication initialized." << std::endl;
    }

    void handleRequest(npos::ipc::Message& message) override {}

    void handleResponse(npos::ipc::Message& message) override
    {
        if (message.type == npos::fs::msg::Type::FILE_OPEN) 
        {
            auto& response = static_cast<npos::fs::msg::file::Open&>(message);

            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                std::cout << "File opened." << std::endl;
                devOid = response.getOpened();

                if(not fileOpenedForReading)
                {
                    std::cout << "Writing to file." << std::endl;
                    npos::fs::api::file::write(*this, devOid, etl::span<const uint8_t>(reinterpret_cast<const uint8_t*>("Hello, World!"), 13));
                }
                else
                {
                    std::cout << "Reading from file." << std::endl;
                    npos::fs::api::file::read(*this, devOid, sizeof(buffer) - 1, etl::span<uint8_t>(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer) - 1));
                }
            }
        }
        else if (message.type == npos::fs::msg::Type::FILE_WRITE) 
        {
            auto& response = static_cast<npos::fs::msg::file::Write&>(message);

            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                std::cout << "File written successfully, now closing file." << std::endl;
                npos::fs::api::file::close(*this, devOid);
            }
        }
        else if (message.type == npos::fs::msg::Type::FILE_CLOSE) 
        {
            auto& response = static_cast<npos::fs::msg::file::Close&>(message);

            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                std::cout << "File closed successfully." << std::endl;

                if(not fileOpenedForReading)
                {
                    std::cout << "Now opening for reading." << std::endl;
                    npos::fs::api::file::open(*this, "hello.txt", npos::fs::Flags::READ_ONLY);
                    fileOpenedForReading = true;
                }
                else
                {
                    std::cout << "Finished reading from file. Bye!" << std::endl;
                }
            }
        }
        else if (message.type == npos::fs::msg::Type::FILE_READ) 
        {
            auto& response = static_cast<npos::fs::msg::file::Read&>(message);

            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                buffer[response.getOutputLength()] = '\0';
                std::cout << "File read successfully: " << buffer << std::endl;

                std::cout << "Closing file after reading." << std::endl;
                npos::fs::api::file::close(*this, devOid);
            }
        }
    }

    bool acceptsRequest(npos::ipc::Message::Type type) const override
    {
        return false;
    }

private:
    etl::string_view devName;
    npos::ipc::Oid devOid;
    bool fileOpenedForReading = false;
    char buffer[128];
};