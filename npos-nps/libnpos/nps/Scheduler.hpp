#pragma once

/**
 * @file Scheduler.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <etl/vector.h>

#include <libnpos/nps/Task.hpp>

namespace npos::nps
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
        using TaskList = etl::ivector<Task*>;

        explicit Scheduler(TaskList&);

        void start();
        bool addTask(Task&);

        bool scheduleTasks() override;
        void initalizeTasks() override;

        inline void setSuccesor(SchedulerSuccessor& ss) { succesor = &ss; }
        inline void stop() { isRunning = false; }

    protected:
        void processIdle();
        void processWatchdog();

    private:
        TaskList& taskList;
        bool isRunning;
        SchedulerSuccessor* succesor;
        Task* idleTask;
        Task* watchdogTask;
    };
}