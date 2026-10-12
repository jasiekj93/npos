#pragma once

/**
 * @file Syslog.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/ipc/service/Syslog.hpp>

namespace npos::cli::program
{
    class Syslog : public Program
    {
    public:
        Syslog(Api& api, const ipc::service::Syslog& collector)
            : Program(api, "syslog", "syslog - print system log")
            , collector(collector)
        {
        }

        void start(Host& terminal, etl::string_view) override
        {
            host = &terminal;
            index = 0;
            offset = 0;
            line.clear();
            pump();
        }

        void resume() override
        {
            pump();
        }

        void handle(ipc::Message&) override {}

    private:
        void pump()
        {
            const auto& entries = collector.getEntries();

            while(index < entries.size())
            {
                if(line.empty())
                    format(entries[index]);

                offset += host->print(etl::string_view(line.c_str() + offset, line.size() - offset));

                if(offset < line.size())
                    return;

                line.clear();
                offset = 0;
                index++;
            }

            host->exit(*this, 0);
        }

        void format(const ipc::service::Syslog::Entry& entry)
        {
            switch(entry.level)
            {
            case ipc::msg::Syslog::ERROR: line = "[E] "; break;
            case ipc::msg::Syslog::WARNING: line = "[W] "; break;
            case ipc::msg::Syslog::INFO: line = "[I] "; break;
            default: line = "[D] "; break;
            }

            line.append(entry.text);
            line.append("\r\n");
        }

        const ipc::service::Syslog& collector;
        Host* host = nullptr;
        size_t index = 0;
        size_t offset = 0;
        etl::string<ipc::service::Syslog::MAX_TEXT + 8> line;
    };
}
