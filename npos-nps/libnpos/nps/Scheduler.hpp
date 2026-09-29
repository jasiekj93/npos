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
        virtual void initializeTasks() = 0;
    };


    class Scheduler : public SchedulerSuccessor
    {
    public:
        using TaskList = etl::ivector<Task*>;

        explicit Scheduler(TaskList& taskList)
            : taskList(taskList)
            , runningFlag(false)
            , succesor(nullptr)
            , idleTask(nullptr)
            , watchdogTask(nullptr)
        {
        }

        void start()
        {
            initialize();

            runningFlag = true;

            while(runningFlag)
            {
                processWatchdog();

                if(not scheduleTasks())
                {
                    if(succesor)
                    {
                        if(not succesor->scheduleTasks())
                            processIdle();
                    }
                    else
                        processIdle();
                }
            }
        }

        bool addTask(Task& task)
        {
            if(taskList.full())
                return false;
            
            taskList.push_back(&task);
            return true;
        }

        bool scheduleTasks() override
        {
            for(auto& task : taskList)
            {
                if(task->isReady())
                {
                    task->process();
                    return true;
                }
            }

            return false;
        }

        void initializeTasks() override
        {
            etl::sort(taskList.begin(), taskList.end(), CompareTasks{});

            for(auto& task : taskList)
                task->initialize();
        }

        inline void setSuccesor(SchedulerSuccessor& ss) { succesor = &ss; }
        inline void stop() { runningFlag = false; }
        inline bool full() const { return taskList.full(); }
        inline bool isRunning() const { return runningFlag; }

    protected:
        void initialize()
        {
            initializeTasks();

            if(succesor)
                succesor->initializeTasks();

            if(watchdogTask)
                watchdogTask->initialize();

            if(idleTask)
                idleTask->initialize();
        }

        void processIdle()
        {
            if(idleTask and idleTask->isReady())
                idleTask->process();
        }

        void processWatchdog()
        {
            if(watchdogTask and watchdogTask->isReady())
                watchdogTask->process();
        }

    private:
        TaskList& taskList;
        bool runningFlag;
        SchedulerSuccessor* succesor;
        Task* idleTask;
        Task* watchdogTask;
    };
}