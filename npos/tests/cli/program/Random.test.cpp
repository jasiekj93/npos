/**
 * @file Random.test.cpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Random.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

namespace
{
    etl::string<512> toHex(const uint8_t* data, size_t size)
    {
        etl::string<512> text;

        for(size_t i = 0; i < size; i++)
            npos::cli::utils::text::appendHex(text, data[i]);

        return text;
    }

    // Output without the line breaks.
    etl::string<512> glue(const etl::istring& output)
    {
        etl::string<512> text;

        for(auto c : output)
            if(c != '\r' and c != '\n')
                text.push_back(c);

        return text;
    }

    // Output of a fresh generator for `size` bytes with the lines glued together.
    etl::string<512> printed(uint32_t seed, const char* size)
    {
        mock::RecordingApi api;
        mock::FakeHost host;
        npos::cli::program::Random random(api, seed);

        random.start(host, size);

        return glue(host.output);
    }
}

TEST_GROUP(Random)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Random random{api};
    mock::ProgramHarness filesystem{random, api};

    void assertUsage()
    {
        CHECK_TRUE(host.outputIs("usage: random <size> [> | >> <file>]\r\n"));
        CHECK_EQUAL(1, host.exitCode);
        CHECK_FALSE(filesystem.hasRequest());
    }

    // Opens the output file and returns everything the program writes into it.
    etl::string<512> writeToFile(const char* args)
    {
        etl::string<512> bytes;
        random.start(host, args);
        filesystem.answerOpen(msg::Status::SUCCESS, { 5, 3 });

        while(filesystem.nextType() == msg::Type::FILE_WRITE)
        {
            auto& request = api.sent.front();
            bytes.append(toHex(request.input.data, request.input.size));
            filesystem.answerWrite();
        }

        filesystem.answerClose();
        return bytes;
    }
};

TEST(Random, HasNameAndDescription)
{
    CHECK_TRUE(random.getName() == "random");
    CHECK_TRUE(random.getDescription().size() > 0);
}

TEST(Random, UsageWithoutArguments)
{
    random.start(host, "");

    assertUsage();
}

TEST(Random, UsageWhenSizeIsNotANumber)
{
    random.start(host, "abc");

    assertUsage();
}

TEST(Random, UsageWhenSizeIsNegative)
{
    random.start(host, "-5");

    assertUsage();
}

TEST(Random, UsageWhenOnlyARedirectIsGiven)
{
    random.start(host, "> f.txt");

    assertUsage();
}

TEST(Random, UsageWhenTheFileIsMissing)
{
    random.start(host, "4 >");

    assertUsage();
}

TEST(Random, UsageWhenTheSizeDoesNotFit)
{
    random.start(host, "99999999999");

    assertUsage();
}

TEST(Random, PrintsTheBytesAsHex)
{
    random.start(host, "4");

    CHECK_EQUAL(8 + 2, host.output.size());
    CHECK_TRUE(host.output.find_first_not_of("0123456789abcdef\r\n") == etl::string<2048>::npos);
    CHECK_EQUAL(0, host.exitCode);
    POINTERS_EQUAL(&random, host.exited);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Random, SizeZeroPrintsNothing)
{
    random.start(host, "0");

    CHECK_TRUE(host.outputIs(""));
    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Random, PrintsLinesOfThirtyTwoBytes)
{
    random.start(host, "40");

    CHECK_EQUAL(64 + 2 + 16 + 2, host.output.size());
    CHECK_TRUE(host.output[64] == '\r' and host.output[65] == '\n');
    CHECK_TRUE(host.output[82] == '\r' and host.output[83] == '\n');
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Random, ByteCountIsTrimmed)
{
    random.start(host, "  3  ");

    CHECK_EQUAL(6 + 2, host.output.size());
}

TEST(Random, EveryRunGivesOtherBytes)
{
    random.start(host, "16");
    auto first = host.output;
    host.output.clear();

    random.start(host, "16");

    CHECK_TRUE(first != host.output);
}

TEST(Random, SameSeedGivesTheSameBytes)
{
    CHECK_TRUE(printed(1234, "24") == printed(1234, "24"));
    CHECK_TRUE(printed(1234, "24") != printed(4321, "24"));
}

TEST(Random, PrefixOfALongerRunIsTheShorterRun)
{
    auto longer = printed(99, "40");
    auto shorter = printed(99, "7");

    CHECK_TRUE(longer.substr(0, shorter.size()) == shorter);
}

TEST(Random, BytesAreNotAllEqual)
{
    auto text = printed(7, "32");
    bool different = false;

    for(size_t i = 2; i < text.size(); i += 2)
        if(text[i] != text[0] or text[i + 1] != text[1])
            different = true;

    CHECK_TRUE(different);
}

TEST(Random, ContinuesWhenTheOutputIsDrained)
{
    host.capacity = 10;
    random.start(host, "40");

    CHECK_EQUAL(10, host.output.size());
    CHECK_EQUAL(0, host.exitCount);

    host.capacity = 2048;
    random.resume();

    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(printed(npos::cli::program::Random::DEFAULT_SEED, "40") == glue(host.output));
}

TEST(Random, ResumeWithoutPendingOutputDoesNothing)
{
    random.resume();

    CHECK_EQUAL(0, host.exitCount);
    CHECK_TRUE(host.outputIs(""));

    random.start(host, "2");
    random.resume();

    CHECK_EQUAL(1, host.exitCount);
}

TEST(Random, TruncatesTheFileWithSingleRedirect)
{
    host.cwd = "/d";

    random.start(host, "8 > r.bin");

    auto request = filesystem.take();
    auto& open = reinterpret_cast<msg::file::Open&>(request);
    STRCMP_EQUAL("/d/r.bin", open.getPath().data());
    CHECK_EQUAL(Flags::WRITE_ONLY | Flags::CREATE | Flags::TRUNCATE, open.getFlags());
    CHECK_TRUE(host.outputIs(""));
}

TEST(Random, AppendsWithDoubleRedirect)
{
    random.start(host, "8 >> r.bin");

    auto request = filesystem.take();
    auto& open = reinterpret_cast<msg::file::Open&>(request);
    CHECK_EQUAL(Flags::WRITE_ONLY | Flags::CREATE | Flags::APPEND, open.getFlags());
}

TEST(Random, WritesTheRequestedNumberOfBytesThenClosesTheFile)
{
    random.start(host, "8 > r.bin");
    filesystem.answerOpen(msg::Status::SUCCESS, { 5, 3 });

    CHECK_EQUAL(msg::Type::FILE_WRITE, filesystem.nextType());
    CHECK_EQUAL(3, api.sent.front().recipient.serviceId);
    CHECK_EQUAL(8, api.sent.front().input.size);

    filesystem.answerWrite();

    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerClose();

    POINTERS_EQUAL(&random, host.exited);
    CHECK_EQUAL(0, host.exitCode);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Random, WritesLargeSizesInChunks)
{
    random.start(host, "150 > r.bin");
    filesystem.answerOpen(msg::Status::SUCCESS);

    CHECK_EQUAL(64, api.sent.front().input.size);
    filesystem.answerWrite();
    CHECK_EQUAL(64, api.sent.front().input.size);
    filesystem.answerWrite();
    CHECK_EQUAL(22, api.sent.front().input.size);
    filesystem.answerWrite();

    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
}

TEST(Random, FileGetsTheSameBytesAsTheTerminal)
{
    auto bytes = writeToFile("100 > r.bin");

    CHECK_EQUAL(200, bytes.size());
    CHECK_TRUE(bytes == printed(npos::cli::program::Random::DEFAULT_SEED, "100"));
}

TEST(Random, SizeZeroCreatesAnEmptyFile)
{
    random.start(host, "0 > r.bin");
    filesystem.answerOpen(msg::Status::SUCCESS);

    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());

    filesystem.answerClose();

    CHECK_EQUAL(0, host.exitCode);
}

TEST(Random, ReportsAFileThatCannotBeOpened)
{
    random.start(host, "8 > r.bin");

    filesystem.answerOpen(msg::Status::NO_SPACE_LEFT);

    CHECK_TRUE(host.outputIs("random: /r.bin: no space left\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Random, ReportsAFailedWriteAndClosesTheFile)
{
    random.start(host, "8 > r.bin");
    filesystem.answerOpen(msg::Status::SUCCESS);

    filesystem.answerWrite(msg::Status::IO_ERROR);

    CHECK_TRUE(host.outputIs("random: /r.bin: I/O error\r\n"));
    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerClose();

    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Random, FailedWriteStopsFurtherWrites)
{
    random.start(host, "150 > r.bin");
    filesystem.answerOpen(msg::Status::SUCCESS);

    filesystem.answerWrite(msg::Status::NO_SPACE_LEFT);

    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(1, api.sent.size());
}

TEST(Random, RejectsAPathThatIsTooLong)
{
    etl::string<160> args("8 > ");
    args.append(80, 'a');

    random.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("random: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Random, IgnoresFileAnswersWhenIdle)
{
    npos::ipc::Message message;
    message.type = msg::Type::FILE_WRITE;

    random.handleResponse(message);
    message.type = msg::Type::FILE_CLOSE;
    random.handleResponse(message);

    CHECK_EQUAL(0, host.exitCount);
}
