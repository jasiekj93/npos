#pragma once

#include <libnpos/ipc/Service.hpp>

namespace npos::ipc
{
    class FsmService : public Service
    {
    public:
        class State
        {
        public:
            using ID = uint8_t;
            static constexpr ID NO_STATE_CHANGE = 0xFF;

            State(ID id, FsmService& fsm) : id(id), fsm(fsm) {}
            virtual ~State() = default;

            virtual void onEnter() {}
            virtual void onExit() {}

            virtual bool acceptsRequest(Message::Type) const { return false; }

            virtual ID handleRequest(Message&) { return id; }
            virtual ID handleResponse(Message&) { return id; }
            inline auto getId() const { return id; }

        protected:
            FsmService& getContext() { return fsm; }

        private:
            ID id;
            FsmService& fsm;
        };

        FsmService(Api& api, State* states[]) 
            : Service(api)
            , states(states) 
        {
        }

        bool acceptsRequest(Message::Type type) const override
        {
            if(not isRunning)
                return false;

            if(currentState == nullptr)
                return false;

            return currentState->acceptsRequest(type);
        }

        void handleRequest(Message& message) override
        {
            if(not isRunning)
                return;

            if(acceptsRequest(message.type))
                return setState(currentState->handleRequest(message));
        }

        void handleResponse(Message& message) override
        {
            if(not isRunning)
                return;

            if(currentState != nullptr)
                setState(currentState->handleResponse(message));
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