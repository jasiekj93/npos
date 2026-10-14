/**
 * @file Task.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeTask.hpp"

#include <CppUTest/TestHarness.h>

TEST_GROUP(Task)
{
    mock::FakeTask::Log log;
};

TEST(Task, KeepsPriority)
{
    mock::FakeTask task(7, 1, log);

    CHECK_EQUAL(7, task.getPriority());
}

TEST(Task, InitializeIsCallable)
{
    mock::FakeTask task(1, 1, log);

    task.initialize();

    CHECK_EQUAL(1, task.initializeCount);
}

TEST(Task, CompareTasksOrdersByPriorityAscending)
{
    mock::FakeTask low(1, 1, log);
    mock::FakeTask high(9, 2, log);
    npos::nps::CompareTasks compare;

    CHECK_TRUE(compare(&low, &high));
    CHECK_FALSE(compare(&high, &low));
    CHECK_FALSE(compare(&low, &low));
}
