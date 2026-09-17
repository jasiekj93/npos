#pragma once

/**
 * @file Task.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <cstdint>

namespace npos::nps
{
    class Task
    {
    public:
        using Priority = uint8_t;

        explicit Task(Priority p) 
            : priority(p) 
        {}

        virtual void initalize() {}
        virtual bool isReady() const = 0;
        virtual void process() = 0;

        inline auto getPriority() const { return priority; }

        bool operator<(const Task& other) const
        {
            return (priority < other.priority);
        }

    private:
        Priority priority;
    };
}