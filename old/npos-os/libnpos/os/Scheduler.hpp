#pragma once

/**
 * @file Scheduler.hpp
 * @author Adrian Szczepanski
 * @date 25-09-2026
 */

#include <etl/vector.h>

#include <libnpos/os/Task.hpp>

namespace npos::os
{
    class SchedulerSuccessor
    {
    public:
        virtual ~SchedulerSuccessor() = default;

        virtual bool scheduleTasks() = 0;
        virtual void initalizeTasks() = 0;
    };

    class Scheduler : public SchedulerSuccessor
    {
    public:
        using TaskQueue = etl::ivector<Task*>;

        explicit Scheduler(TaskQueue&);

        bool addTask(Task& task);

        bool scheduleTasks() override;
        void initalizeTasks() override;

        void start();
        inline bool isRunning() const { return runningFlag; }
        inline void stop() { runningFlag = false; }

        inline void setSuccessor(SchedulerSuccessor* succ) { successor = succ; }
        inline void setIdleTask(Task* task) { idleTask = task; }
        inline void setWatchdogTask(Task* task) { watchdogTask = task; }

    private:
        TaskQueue& taskQueue;
        SchedulerSuccessor* successor = nullptr;
        Task* idleTask = nullptr;
        Task* watchdogTask = nullptr;
        bool runningFlag = false;
    };
}