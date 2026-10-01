#include "Scheduler.hpp"

using namespace npos::os;

Scheduler::Scheduler(TaskQueue& queue)
    : taskQueue(queue)
{
}

bool Scheduler::addTask(Task& task)
{
    if(taskQueue.full())
        return false;

    taskQueue.push_back(&task);
    return true;
}

bool Scheduler::scheduleTasks()
{
    // Implement the scheduling logic here
    return true;
}

void Scheduler::initalizeTasks()
{
    etl::sort(taskQueue.begin(), taskQueue.end(), CompareTask());

    if(idleTask)
        idleTask->initalize();

    if(watchdogTask)
        watchdogTask->initalize();

    for(auto task : taskQueue)
        task->initalize();

    if(successor)
        successor->initalizeTasks();
}

void Scheduler::start()
{
    initalizeTasks();
    
    runningFlag = true;

    while(runningFlag)
    {
        if(watchdogTask and watchdogTask->isReady())
            watchdogTask->process();

        auto isScheduled = scheduleTasks();

        if(not isScheduled)
        {
            if(successor)
                isScheduled = successor->scheduleTasks();

            if(not isScheduled)
            {
                if(idleTask and idleTask->isReady())
                    idleTask->process();
            }
        }
    }
}
