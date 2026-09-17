#pragma once

/**
 * @file BlinkingLed.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <etl/queue.h>

#include <libnpos/os/driver/message/Led.hpp>
#include <libnpos/os/Process.hpp>

class BlinkingLed : public npos::os::Process
{
public:
    BlinkingLed(Priority priority, npos::os::message::Bus& bus) 
        : npos::os::Process(priority)
        , bus(bus)
    {}

    void onReceive(const Message&) override;
    bool accepts(Message::Id) const override;

    void initalize() override;
    bool isReady() const override;
     void process() override;

private:
    npos::os::message::Bus& bus;
};