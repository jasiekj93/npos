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
        inline void stop() { runningFlag = false; }
        inline bool full() const { return taskList.full(); }
        inline bool isRunning() const { return runningFlag; }

    protected:
        void processIdle();
        void processWatchdog();

    private:
        TaskList& taskList;
        bool runningFlag;
        SchedulerSuccessor* succesor;
        Task* idleTask;
        Task* watchdogTask;
    };
}