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
        npos::fs::api::file::open(*this, "hello.txt", { npos::fs::Flag::WRITE_ONLY | npos::fs::Flag::CREATE });
        std::cout << "UserApplication initialized." << std::endl;
    }

    void handle(npos::ipc::Message& message) override
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
                    npos::fs::api::file::write(*this, devOid, "Hello, World!", 13);
                }
                else
                {
                    std::cout << "Reading from file." << std::endl;
                    npos::fs::api::file::read(*this, devOid, 128, buffer);
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
                    npos::fs::api::file::open(*this, "hello.txt", { npos::fs::Flag::READ_ONLY });
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
                std::cout << "File read successfully: " << buffer << std::endl;
                buffer[response.getOutputLength()] = '\0';
                std::cout << "Buffer after null-termination: " << buffer << std::endl;

                std::cout << "Closing file after reading." << std::endl;
                npos::fs::api::file::close(*this, devOid);
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
    bool fileOpenedForReading = false;
    char buffer[128];
};