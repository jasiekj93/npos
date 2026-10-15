/**
 * @file Pwd.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/cli/program/Pwd.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Pwd)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Pwd pwd{api};
};

TEST(Pwd, HasNameAndDescription)
{
    CHECK_TRUE(pwd.getName() == "pwd");
    CHECK_TRUE(pwd.getDescription().size() > 0);
}

TEST(Pwd, PrintsWorkingDirectoryAndExits)
{
    host.cwd = "/a/b";

    pwd.start(host, "");

    CHECK_TRUE(host.outputIs("/a/b\r\n"));
    POINTERS_EQUAL(&pwd, host.exited);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Pwd, IgnoresMessages)
{
    npos::ipc::Message message;

    pwd.handleResponse(message);

    CHECK_EQUAL(0, api.sent.size());
}
