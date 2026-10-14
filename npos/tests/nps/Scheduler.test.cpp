/**
 * @file Scheduler.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeTask.hpp"

#include <CppUTest/TestHarness.h>

TEST_GROUP(Scheduler)
{
    mock::FakeTask::Log processLog;
    mock::FakeTask::Log initializeLog;
    npos::nps::Scheduler::TaskList<4> taskList;
    npos::nps::Scheduler scheduler{taskList};
};

TEST(Scheduler, IsNotRunningInitially)
{
    CHECK_FALSE(scheduler.isRunning());
}

TEST(Scheduler, AddTaskFailsWhenFull)
{
    mock::FakeTask a(1, 1, processLog), b(2, 2, processLog), c(3, 3, processLog), d(4, 4, processLog), e(5, 5, processLog);

    CHECK_FALSE(scheduler.full());
    CHECK_TRUE(scheduler.addTask(a));
    CHECK_TRUE(scheduler.addTask(b));
    CHECK_TRUE(scheduler.addTask(c));
    CHECK_TRUE(scheduler.addTask(d));
    CHECK_TRUE(scheduler.full());
    CHECK_FALSE(scheduler.addTask(e));
}

TEST(Scheduler, ScheduleTasksReturnsFalseWhenNoTaskIsReady)
{
    mock::FakeTask a(1, 1, processLog);
    a.ready = false;
    scheduler.addTask(a);

    CHECK_FALSE(scheduler.scheduleTasks());
    CHECK_EQUAL(0, a.processCount);
}

TEST(Scheduler, ScheduleTasksReturnsFalseWithoutTasks)
{
    CHECK_FALSE(scheduler.scheduleTasks());
}

TEST(Scheduler, ScheduleTasksProcessesOnlyFirstReadyTask)
{
    mock::FakeTask a(1, 1, processLog), b(2, 2, processLog), c(3, 3, processLog);
    a.ready = false;
    scheduler.addTask(a);
    scheduler.addTask(b);
    scheduler.addTask(c);

    CHECK_TRUE(scheduler.scheduleTasks());

    CHECK_EQUAL(0, a.processCount);
    CHECK_EQUAL(1, b.processCount);
    CHECK_EQUAL(0, c.processCount);
}

TEST(Scheduler, InitializeTasksSortsByPriorityAndInitializesAll)
{
    mock::FakeTask a(5, 1, processLog, &initializeLog), b(1, 2, processLog, &initializeLog), c(3, 3, processLog, &initializeLog);
    scheduler.addTask(a);
    scheduler.addTask(b);
    scheduler.addTask(c);

    scheduler.initializeTasks();

    CHECK_EQUAL(3, initializeLog.size());
    CHECK_EQUAL(2, initializeLog[0]);
    CHECK_EQUAL(3, initializeLog[1]);
    CHECK_EQUAL(1, initializeLog[2]);
}

TEST(Scheduler, StartRunsUntilStopped)
{
    mock::FakeTask a(1, 1, processLog);
    a.stopOnProcess = &scheduler;
    scheduler.addTask(a);

    scheduler.start();

    CHECK_EQUAL(1, a.initializeCount);
    CHECK_EQUAL(1, a.processCount);
    CHECK_FALSE(scheduler.isRunning());
}

TEST(Scheduler, IsRunningWhileProcessing)
{
    struct RunningCheck : public mock::FakeTask
    {
        using mock::FakeTask::FakeTask;
        npos::nps::Scheduler* observed = nullptr;
        bool wasRunning = false;

        void process() override
        {
            wasRunning = observed->isRunning();
            mock::FakeTask::process();
        }
    };

    RunningCheck task(1, 1, processLog);
    task.observed = &scheduler;
    task.stopOnProcess = &scheduler;
    scheduler.addTask(task);

    scheduler.start();

    CHECK_TRUE(task.wasRunning);
}

TEST(Scheduler, HigherPriorityTaskRunsFirst)
{
    bool stayReady = false;
    mock::FakeTask low(9, 9, processLog), high(1, 1, processLog);
    low.readyAfterProcess = &stayReady;
    high.readyAfterProcess = &stayReady;
    low.stopOnProcess = &scheduler;
    scheduler.addTask(low);
    scheduler.addTask(high);

    scheduler.start();

    CHECK_EQUAL(2, processLog.size());
    CHECK_EQUAL(1, processLog[0]);
    CHECK_EQUAL(9, processLog[1]);
}

TEST(Scheduler, SuccessorIsInitializedAndScheduledWhenPrimaryIsIdle)
{
    npos::nps::Scheduler::TaskList<2> successorTasks;
    npos::nps::Scheduler successor(successorTasks);
    mock::FakeTask primaryTask(1, 1, processLog, &initializeLog);
    mock::FakeTask successorTask(1, 2, processLog, &initializeLog);
    primaryTask.ready = false;
    successorTask.stopOnProcess = &scheduler;
    scheduler.addTask(primaryTask);
    successor.addTask(successorTask);
    scheduler.setSuccesor(successor);

    scheduler.start();

    CHECK_EQUAL(2, initializeLog.size());
    CHECK_EQUAL(1, successorTask.processCount);
    CHECK_EQUAL(0, primaryTask.processCount);
}

TEST(Scheduler, IdleLoopSpinsWithoutTasksUntilStopped)
{
    struct StopAfterChecks : public mock::FakeTask
    {
        using mock::FakeTask::FakeTask;
        int checks = 0;

        bool isReady() const override
        {
            // const: count through a mutable alias of the test state
            const_cast<StopAfterChecks*>(this)->checks++;

            if(checks > 3)
                stopOnProcess->stop();

            return false;
        }
    };

    StopAfterChecks task(1, 1, processLog);
    task.stopOnProcess = &scheduler;
    scheduler.addTask(task);

    scheduler.start();

    CHECK_TRUE(task.checks > 3);
    CHECK_EQUAL(0, task.processCount);
}
