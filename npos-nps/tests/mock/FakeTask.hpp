#pragma once

/**
 * @file FakeTask.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/nps/Scheduler.hpp>

namespace mock
{
    class FakeTask : public npos::nps::Task
    {
    public:
        using Log = etl::vector<int, 32>;

        FakeTask(Priority priority, int id, Log& processLog, Log* initializeLog = nullptr)
            : npos::nps::Task(priority)
            , id(id)
            , processLog(processLog)
            , initializeLog(initializeLog)
        {
        }

        void initialize() override
        {
            initializeCount++;

            if(initializeLog)
                initializeLog->push_back(id);
        }

        bool isReady() const override { return ready; }

        void process() override
        {
            processCount++;
            processLog.push_back(id);

            if(stopOnProcess)
                stopOnProcess->stop();

            if(readyAfterProcess)
                ready = *readyAfterProcess;
        }

        int id;
        bool ready = true;
        const bool* readyAfterProcess = nullptr;
        int initializeCount = 0;
        int processCount = 0;
        npos::nps::Scheduler* stopOnProcess = nullptr;

    private:
        Log& processLog;
        Log* initializeLog;
    };
}
