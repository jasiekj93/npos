#pragma once

/**
 * @file Fsm.hpp
 * @author Adrian Szczepanski
 * @date 03-09-2026
 */

#include <libnpos/ipc/MessageRouter.hpp>

namespace npos::ipc
{
    class Fsm : public MessageRouter
    {
    public:
        class State
        {
        public:
            using ID = uint8_t;
            static constexpr ID NO_STATE_CHANGE = 0xFF;

            State(ID id, Fsm& fsm) : id(id), fsm(fsm) {}
            virtual ~State() = default;

            virtual void onEnter() {}
            virtual void onExit() {}

            virtual bool accepts(Message::Id) const { return false; }

            virtual ID process(const Message&) { return id; }
            virtual ID processUnknown(const Message&) { return id; }
            inline auto getId() const { return id; }

        protected:
            Fsm& getContext() { return fsm; }

        private:
            ID id;
            Fsm& fsm;
        };

        Fsm(MessageRouter::Id id, State* states[]) 
            : MessageRouter(id)
            , states(states) 
        {
        }

        bool accepts(Message::Id messageId) const override
        {
            if(not isRunning)
                return false;

            if(currentState == nullptr)
                return false;

            return currentState->accepts(messageId);
        }

        void receive(const Message& message) override
        {
            if(not isRunning)
                return;

            if(not accepts(message.getId()))
                return setState(currentState->processUnknown(message));

            setState(currentState->process(message));  
        }

        void start()
        {
            if(states == nullptr or states[0] == nullptr)
                return;

            isRunning = true;
            currentState = states[0];
            currentState->onEnter();
        }

        void setState(State::ID id)
        {
            if(not isRunning)
                return;

            if(currentState->getId() == id or 
                id == State::NO_STATE_CHANGE) 
                return;

            currentState->onExit();
            currentState = states[id];
            currentState->onEnter();
        }

        inline auto getCurrentState() const { return currentState; }

    private:
        State** states;
        State* currentState = nullptr;
        bool isRunning = false;
    };
}