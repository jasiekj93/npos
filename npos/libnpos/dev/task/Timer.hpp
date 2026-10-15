#pragma once

/**
 * @file Timer.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <etl/priority_queue.h>

#include <libnpos/dev/msg/Timer.hpp>
#include <libnpos/nps/Task.hpp>
#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/dev/hal/SystemTick.hpp>

namespace npos::dev::task
{
    struct CompareTimerMessage : public etl::binary_function<msg::Timer*, msg::Timer*, bool>
    {
        // less
        bool operator()(const msg::Timer* lhs, const msg::Timer* rhs) const
        {
            return lhs->getEventTick() < rhs->getEventTick();
        }
    };

    class Timer : public nps::Task, public ipc::Port
    {
    public:
        using MessageQueueInt = etl::ipriority_queue<msg::Timer*, etl::ivector<msg::Timer*>, CompareTimerMessage>;

        template <size_t N>
        using MessageQueue = etl::priority_queue<msg::Timer*, N, etl::vector<msg::Timer*, N>, CompareTimerMessage>;

        Timer(Priority priority, MessageQueueInt& messageQueue, ipc::Bus& bus, hal::SystemTick& systemTick)
            : nps::Task(priority)
            , messageQueue(messageQueue)
            , systemTick(systemTick)
            , bus(bus)
        {
        }

        //Port
        bool accepts(ipc::Message::Type type) const override
        {
            return type == msg::Type::TIMER;
        }

        void receiveRequest(const ipc::Message& message) override
        {
            if(message.type != msg::Type::TIMER)
            {
                auto& unknown = const_cast<ipc::Message&>(message);
                handleUnknownRequest(unknown);
                return bus.release(&unknown);
            }

            auto& timerMessage = const_cast<msg::Timer&>(static_cast<const msg::Timer&>(message));
            if(messageQueue.full())
            {
                bus.respond(timerMessage.setStatus(msg::Timer::Status::NO_MEMORY));
                bus.release(&timerMessage);
            }
            else
            {
                timerMessage.setEventTick(systemTick.getTick() + timerMessage.getInterval());
                timerMessage.recipient = timerMessage.getTarget();
                messageQueue.push(&timerMessage);
            }
        }

        // The timer sends no requests, so there is nothing to answer.
        void receiveResponse(const ipc::Message& message) override
        {
            bus.release(const_cast<ipc::Message*>(&message));
        }

        void handleUnknownRequest(ipc::Message& message) override
        {
            message.output.status = ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED;
            bus.respond(message);
        }

        //nps::Task
        bool isReady() const override
        {
            if(messageQueue.empty())
                return false;
            else
                return messageQueue.top()->getEventTick() <= systemTick.getTick();
        }
        
        void process() override
        {
            if(messageQueue.empty())
                return;

            auto& timerMessage = messageQueue.top();

            if(timerMessage->getEventTick() > systemTick.getTick())
                return;
                
            bus.respond(timerMessage->setStatus(msg::Timer::Status::SUCCESS));
            messageQueue.pop();

            if(timerMessage->getMode() == msg::Timer::Mode::REPEATING)
            {
                timerMessage->setEventTick(systemTick.getTick() + timerMessage->getInterval());
                messageQueue.push(timerMessage);
            }
            else
                bus.release(timerMessage);
        }

    private:
        MessageQueueInt& messageQueue;
        hal::SystemTick& systemTick;
        ipc::Bus& bus;
    };
}