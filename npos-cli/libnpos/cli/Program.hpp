#pragma once

/**
 * @file Program.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstddef>

#include <etl/string_view.h>

#include <libnpos/ipc/Service.hpp>

namespace npos::cli
{
    // A program is a service subscribed to the terminal's task, so it receives the responses to its own requests.
    class Program : public ipc::Service
    {
    public:
        class Host
        {
        public:
            virtual ~Host() = default;

            // Returns the number of characters accepted; the rest must be sent again after resume().
            virtual size_t print(etl::string_view text) = 0;
            virtual void exit(Program& program, int code) = 0;
        };

        Program(Api& api, etl::string_view name, etl::string_view description)
            : ipc::Service(api)
            , name(name)
            , description(description)
        {
        }

        // Args stay valid only during the call.
        virtual void start(Host& host, etl::string_view args) = 0;

        // Called when the terminal output has been drained.
        virtual void resume() {}

        // Programs only consume responses.
        bool accepts(ipc::Message::Type) const override { return false; }

        inline etl::string_view getName() const { return name; }
        inline etl::string_view getDescription() const { return description; }

    private:
        etl::string_view name;
        etl::string_view description;
    };
}
