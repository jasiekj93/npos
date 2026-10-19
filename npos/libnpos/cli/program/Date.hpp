#pragma once

/**
 * @file Date.hpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/dev/Api.hpp>

namespace npos::cli::program
{
    // date prints the time of the clock device, date -s <YYYY-MM-DD [HH:MM[:SS]]> sets it.
    class Date : public Program
    {
    public:
        // Nobody answers an open of a device that does not exist, so the program gives up after this time.
        static constexpr dev::Tick OPEN_TIMEOUT = 1000;

        Date(Api& api, etl::string_view clockName)
            : Program(api, "date", "date [-s YYYY-MM-DD [HH:MM[:SS]]] - show or set the date and time")
            , clockName(clockName)
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            setting = false;

            auto rest = utils::text::trim(args);

            if(not rest.empty())
            {
                utils::text::DateTime dateTime;
                auto option = utils::text::token(rest);

                if(not (option == "-s") or
                    not utils::text::parseDateTime(rest, dateTime) or
                    not utils::text::toTimestamp(dateTime, timestamp))
                {
                    host->print("usage: date [-s YYYY-MM-DD [HH:MM[:SS]]]\r\n");
                    return host->exit(*this, 1);
                }

                setting = true;
            }

            if(opened)
                return request();

            state = State::OPENING;
            dev::api::open(*this, clockName);
            dev::api::timerRegister(*this, dev::msg::Timer::Mode::SINGLE_SHOT, OPEN_TIMEOUT);
            timers++;
        }

        void handleResponse(ipc::Message& message) override
        {
            switch(message.type)
            {
            case dev::msg::Type::OPEN:
                return onOpen(static_cast<dev::msg::Open&>(message));
            case dev::msg::Type::TIMER:
                return onTimer();
            case dev::msg::Type::CLOCK:
                return onClock(static_cast<dev::msg::Clock&>(message));
            default:
                return;
            }
        }

    private:
        enum class State { IDLE, OPENING, WORKING };

        void onOpen(dev::msg::Open& response)
        {
            if(state != State::OPENING)
                return;

            if(response.getStatus() != dev::msg::Open::Status::SUCCESS)
            {
                host->print("date: cannot open ");
                host->print(clockName);
                host->print("\r\n");
                return finish(1);
            }

            clock = response.getOpened();
            opened = true;
            request();
        }

        // Every run leaves one timer behind, only the last answer belongs to the run that waits.
        void onTimer()
        {
            if(timers > 0)
                timers--;

            if(timers == 0 and state == State::OPENING)
            {
                host->print("date: ");
                host->print(clockName);
                host->print(": no such device\r\n");
                finish(1);
            }
        }

        void onClock(dev::msg::Clock& response)
        {
            if(state != State::WORKING)
                return;

            if(response.getStatus() != dev::msg::Clock::Status::SUCCESS)
            {
                host->print("date: ");
                host->print(describe(response.getStatus()));
                host->print("\r\n");
                return finish(1);
            }

            etl::string<32> text;
            utils::text::formatDateTime(text, utils::text::toDateTime(response.getOutputTimestamp()));
            text.append("\r\n");
            host->print(etl::string_view(text.data(), text.size()));
            finish(0);
        }

        void request()
        {
            state = State::WORKING;

            if(setting)
                dev::api::setTime(*this, clock, timestamp);
            else
                dev::api::getTime(*this, clock);
        }

        void finish(int code)
        {
            state = State::IDLE;
            host->exit(*this, code);
        }

        static const char* describe(dev::msg::Clock::Status status)
        {
            switch(status)
            {
            case dev::msg::Clock::Status::DEVICE_FAILURE: return "clock failure";
            case dev::msg::Clock::Status::INVALID_TIMESTAMP: return "date out of the supported range";
            case dev::msg::Clock::Status::NOT_INITIALIZED: return "clock not initialized";
            default: return "unknown error";
            }
        }

        etl::string_view clockName;
        Host* host = nullptr;
        State state = State::IDLE;
        ipc::Oid clock = ipc::NULL_OID;
        bool opened = false;
        bool setting = false;
        uint64_t timestamp = 0;
        size_t timers = 0;
    };
}
