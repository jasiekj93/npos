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
        bool operator()(const msg::Timer* lhs, const msg::Timer* rhs) const
        {
            return lhs->getInterval() < rhs->getInterval();
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

        void receive(const ipc::Message& message) override
        {
            if(message.type != msg::Type::TIMER)
                return;

            auto& timerMessage = const_cast<msg::Timer&>(static_cast<const msg::Timer&>(message));
            if(messageQueue.full())
            {
                timerMessage.isResponse = true;
                bus.send(timerMessage.setStatus(msg::Timer::Status::NO_MEMORY));
                bus.release(&timerMessage);
            }
            else
            {
                timerMessage.setEventTick(systemTick.getTick() + timerMessage.getInterval());
                timerMessage.recipient = timerMessage.getTarget();
                messageQueue.push(&timerMessage);
            }
        }

        //nps::Task
        bool isReady() const override
        {
            if(messageQueue.empty())
                return false;
            else
                return messageQueue.top()->getInterval() <= systemTick.getTick();
        }
        
        void process() override
        {
            if(messageQueue.empty())
                return;

            auto& timerMessage = messageQueue.top();

            if(timerMessage->getEventTick() > systemTick.getTick())
                return;
                
            timerMessage->isResponse = true;
            bus.send(timerMessage->setStatus(msg::Timer::Status::SUCCESS));
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