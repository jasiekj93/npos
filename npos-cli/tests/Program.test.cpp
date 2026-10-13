/**
 * @file Program.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/FakeProgram.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/fs/msg/Type.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Minimal : public npos::cli::Program
    {
    public:
        explicit Minimal(Api& api) : npos::cli::Program(api, "min", "min - minimal") {}
        void start(Host&, etl::string_view) override { started = true; }
        void handle(npos::ipc::Message&) override {}
        bool started = false;
    };
}

TEST_GROUP(Program)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    Minimal program{api};
};

TEST(Program, ExposesNameAndDescription)
{
    CHECK_TRUE(program.getName() == "min");
    CHECK_TRUE(program.getDescription() == "min - minimal");
}

TEST(Program, ConsumesOnlyResponsesSoAcceptsNothing)
{
    CHECK_FALSE(program.accepts(npos::fs::msg::Type::FILE_OPEN));
    CHECK_FALSE(program.accepts(npos::ipc::OS_SYSLOG_MESSAGE_TYPE));
}

TEST(Program, ResumeDoesNothingByDefault)
{
    program.resume();

    CHECK_EQUAL(0, api.sent.size());
}

TEST(Program, StartRunsThroughInterface)
{
    npos::cli::Program& base = program;

    base.start(host, "");

    CHECK_TRUE(program.started);
}

TEST(Program, HostExitReportsProgramAndCode)
{
    host.exit(program, 3);

    POINTERS_EQUAL(&program, host.exited);
    CHECK_EQUAL(3, host.exitCode);
}

TEST(Program, FakeHostLimitsAcceptedOutput)
{
    host.capacity = 2;

    CHECK_EQUAL(2, host.print("abcd"));
    CHECK_EQUAL(0, host.print("x"));
    CHECK_TRUE(host.outputIs("ab"));
}
