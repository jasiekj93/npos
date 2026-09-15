#include "Task.hpp"

using namespace npos::nps;

Task::Task(Priority p)
    : priority(p)
{
}

bool Task::operator<(const Task& other) const
{
    return (priority < other.priority);
}
