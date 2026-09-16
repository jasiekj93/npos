#include "Scheduler.hpp"

using namespace npos::nps;

Scheduler::Scheduler(TaskList& taskList)
    : taskList(taskList)
{
}

void Scheduler::start()
{
    initalizeTasks();

    if(succesor)
        succesor->initalizeTasks();

    if(watchdogTask)
        watchdogTask->initalize();

    if(idleTask)
        idleTask->initalize();

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

bool Scheduler::addTask(Task& task)
{
    if(taskList.full())
        return false;
    
    taskList.push_back(&task);
    return true;
}

bool Scheduler::scheduleTasks()
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

void Scheduler::initalizeTasks()
{
    etl::sort(taskList.begin(), taskList.end(), 
        [](const Task* lhs, const Task* rhs) {
        return *rhs < *lhs;  // descending
    });

    for(auto& task : taskList)
        task->initalize();
}

void Scheduler::processIdle()
{
    if(idleTask and idleTask->isReady())
        idleTask->process();
}

void Scheduler::processWatchdog()
{
    if(watchdogTask and watchdogTask->isReady())
        watchdogTask->process();
}
