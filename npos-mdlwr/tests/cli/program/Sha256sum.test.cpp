/**
 * @file Sha256sum.test.cpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/cli/program/Sha256sum.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::dev;

namespace
{
    using Hash = msg::FileHash;
}

TEST_GROUP(Sha256sum)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::mdlwr::cli::program::Sha256sum sha256sum{api};
    mock::Filesystem filesystem{sha256sum, api};

    // Answers the oldest request with the status, a digest of the bytes first, second, ... is written.
    void answer(Hash::Status status, uint8_t first = 0)
    {
        auto message = filesystem.take();
        auto& response = reinterpret_cast<Hash&>(message);

        for(size_t i = 0; i < response.getOutputBuffer().size(); i++)
            response.getOutputBuffer()[i] = static_cast<uint8_t>(first + i);

        response.setStatus(status);
        sha256sum.handleResponse(message);
    }
};

TEST(Sha256sum, HasNameAndDescription)
{
    CHECK_TRUE(sha256sum.getName() == "sha256sum");
    CHECK_TRUE(sha256sum.getDescription().size() > 0);
}

TEST(Sha256sum, WithoutOperandsPrintsUsage)
{
    sha256sum.start(host, "");

    CHECK_TRUE(host.outputIs("usage: sha256sum <file>...\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Sha256sum, OptionsAreNotSupported)
{
    sha256sum.start(host, "-c a");

    CHECK_TRUE(host.outputIs("sha256sum: invalid option -- 'c'\r\nusage: sha256sum <file>...\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Sha256sum, RequestsTheDigestOfTheFileRelativeToWorkingDirectory)
{
    host.cwd = "/work";

    sha256sum.start(host, "a.txt");

    CHECK_EQUAL(msg::Type::FILE_HASH, filesystem.nextType());
    STRCMP_EQUAL("/work/a.txt", filesystem.nextPath());
    CHECK_EQUAL(32, api.sent.front().output.size);
}

TEST(Sha256sum, PrintsLowerCaseHexDigestTwoSpacesAndTheNameAsGiven)
{
    sha256sum.start(host, "dir/a.txt");

    answer(Hash::SUCCESS, 0xF0);

    CHECK_TRUE(host.outputIs("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff000102030405060708090a0b0c0d0e0f  dir/a.txt\r\n"));
    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Sha256sum, HandlesManyFilesOneAfterAnother)
{
    sha256sum.start(host, "a b");

    answer(Hash::SUCCESS, 0);
    STRCMP_EQUAL("/b", filesystem.nextPath());
    CHECK_EQUAL(0, host.exitCount);
    answer(Hash::SUCCESS, 1);

    CHECK_TRUE(host.output.find("  a\r\n") != etl::string<2048>::npos);
    CHECK_TRUE(host.output.find("  b\r\n") != etl::string<2048>::npos);
    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Sha256sum, FailureIsReportedAndTheRestIsHashed)
{
    sha256sum.start(host, "a b");

    answer(Hash::FILE_NOT_FOUND);

    CHECK_TRUE(host.outputIs("sha256sum: cannot read 'a': no such file\r\n"));
    STRCMP_EQUAL("/b", filesystem.nextPath());
    answer(Hash::SUCCESS);
    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Sha256sum, ReportsReasonOfEveryStatus)
{
    sha256sum.start(host, "a");
    answer(Hash::IS_A_DIRECTORY);
    CHECK_TRUE(host.outputIs("sha256sum: cannot read 'a': is a directory\r\n"));

    host.output.clear();
    sha256sum.start(host, "a");
    answer(Hash::HASH_NOT_FOUND);
    CHECK_TRUE(host.outputIs("sha256sum: cannot read 'a': hash device not found\r\n"));

    host.output.clear();
    sha256sum.start(host, "a");
    answer(Hash::HASH_BUSY);
    CHECK_TRUE(host.outputIs("sha256sum: cannot read 'a': hash device busy\r\n"));

    host.output.clear();
    sha256sum.start(host, "a");
    answer(Hash::BUSY);
    CHECK_TRUE(host.outputIs("sha256sum: cannot read 'a': service busy\r\n"));
}

TEST(Sha256sum, UnavailableServiceIsReported)
{
    sha256sum.start(host, "a");

    auto message = filesystem.take();
    message.output.status = npos::ipc::Message::STATUS_SERVICE_NOT_FOUND;
    sha256sum.handleResponse(message);

    CHECK_TRUE(host.outputIs("sha256sum: cannot read 'a': service not available\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Sha256sum, TooLongNameIsReported)
{
    etl::string<100> args(70, 'a');

    sha256sum.start(host, args.c_str());

    CHECK_TRUE(host.output.find("name too long") != etl::string<2048>::npos);
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Sha256sum, ContinuesPrintingAfterTheTerminalDrains)
{
    host.capacity = 10;
    sha256sum.start(host, "a b");
    answer(Hash::SUCCESS);

    CHECK_EQUAL(10, host.output.size());
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(0, host.exitCount);

    host.capacity = 2048;
    sha256sum.resume();

    CHECK_TRUE(host.output.find("  a\r\n") != etl::string<2048>::npos);
    STRCMP_EQUAL("/b", filesystem.nextPath());
}

TEST(Sha256sum, ResumeWithNothingToPrintDoesNothing)
{
    sha256sum.start(host, "a");

    sha256sum.resume();

    CHECK_TRUE(host.outputIs(""));
    CHECK_EQUAL(0, host.exitCount);
}

TEST(Sha256sum, CanBeStartedAgain)
{
    sha256sum.start(host, "a");
    answer(Hash::FILE_NOT_FOUND);
    host.output.clear();

    sha256sum.start(host, "b");
    answer(Hash::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_TRUE(host.output.find("  b\r\n") != etl::string<2048>::npos);
}

TEST(Sha256sum, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = msg::Type::FILE_HASH;
    sha256sum.handleResponse(message);

    sha256sum.start(host, "a");
    message.type = npos::fs::msg::Type::FILE_OPEN;
    sha256sum.handleResponse(message);

    CHECK_EQUAL(0, host.exitCount);
    CHECK_EQUAL(1, api.sent.size());
}
