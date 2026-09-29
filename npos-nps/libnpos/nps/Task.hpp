#pragma once

/**
 * @file Task.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <cstdint>

#include <etl/functional.h>

namespace npos::nps
{
    class Task
    {
    public:
        using Priority = uint8_t;

        explicit Task(Priority p) 
            : priority(p) 
        {}

        virtual void initialize() {}
        virtual bool isReady() const = 0;
        virtual void process() = 0;

        inline auto getPriority() const { return priority; }

    private:
        Priority priority;
    };

    struct CompareTasks : public etl::binary_function<Task, Task, bool>
    {
        bool operator()(const Task* lhs, const Task* rhs) const
        {
            return lhs->getPriority() < rhs->getPriority();
        }
    };
}