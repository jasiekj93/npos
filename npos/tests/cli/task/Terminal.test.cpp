/**
 * @file Terminal.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/Console.hpp"
#include "mock/FakeProgram.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/cli/task/Terminal.hpp>
#include <libnpos/cli/utils/LogStream.hpp>

#include <CppUTest/TestHarness.h>

using npos::cli::task::Terminal;

#define CHECK_OUTPUT(expected) STRCMP_EQUAL(expected, console.take().c_str())

TEST_GROUP(Terminal)
{
    static constexpr const char* PROMPT = "npos:/> ";

    mock::RecordingApi api;
    mock::RecordingBus bus;
    Terminal::MessageQueue<8> queue;
    etl::generic_pool<npos::cli::utils::LogStream::MAX_MESSAGE_LENGTH, 1, 4> payloads;
    Terminal::ProgramList<2> programList;
    Terminal terminal{2, queue, payloads, programList, bus, "uart0"};
    mock::Console console{terminal, bus};
    mock::FakeProgram program{api};

    void setup() override { terminal.setId(3); }

    // Text printed when the task starts.
    static etl::string<128> greeting()
    {
        etl::string<128> text("\r\nWelcome to npos ");
        etl::to_string(static_cast<unsigned>(npos::VERSION.major), text, true);
        text.push_back('.');
        etl::to_string(static_cast<unsigned>(npos::VERSION.minor), text, true);
        text.push_back('.');
        etl::to_string(static_cast<unsigned>(npos::VERSION.patch), text, true);
        text.append("\r\nType 'help' to list the available programs.\r\n");
        return text;
    }

    static etl::string<128> banner()
    {
        auto text = greeting();
        text.append(PROMPT);
        return text;
    }

    void start()
    {
        terminal.addProgram(program);
        terminal.initialize();
        console.open();
        console.take();
    }

    // Line as drawn after an edit, with the cursor `back` characters before its end.
    etl::string<256> redraw(const char* line, int back = 0)
    {
        etl::string<256> expected("\r");
        expected.append(PROMPT);
        expected.append(line);
        expected.append("\x1b[K");

        if(back > 0)
        {
            expected.append("\x1b[");
            expected.push_back(static_cast<char>('0' + back));
            expected.push_back('D');
        }

        return expected;
    }

    void run(const char* command)
    {
        console.type(command);
        console.type("\r");
        console.take();
    }

    void log(const char* text)
    {
        npos::ipc::Oid sender{10, 0};
        npos::cli::msg::Syslog message(sender, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, text, false);
        message.sender.portId = 10;
        terminal.receiveRequest(message);
        terminal.process();
        console.pump();
    }
};

TEST(Terminal, InitializeBroadcastsOpenRequestForTheDevice)
{
    terminal.initialize();

    CHECK_EQUAL(1, bus.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::OPEN, bus.sent[0].type);
    CHECK_EQUAL(npos::ipc::NULL_PORT, bus.sent[0].recipient.portId);
    CHECK_TRUE(reinterpret_cast<npos::dev::msg::Open&>(bus.sent[0]).getName() == "uart0");
}

TEST(Terminal, OpenedDeviceShowsBannerAndPromptAndStartsReading)
{
    terminal.initialize();

    console.open();

    CHECK_OUTPUT(banner().c_str());
    CHECK_TRUE(console.reading);
}

TEST(Terminal, ReadRequestAsksForTheDeviceBuffer)
{
    terminal.initialize();
    console.open();

    CHECK_EQUAL(npos::dev::msg::Transciever::READ, console.readRequest.input.readWrite.mode);
    CHECK_EQUAL(Terminal::READ_SIZE, console.readRequest.input.readWrite.length);
    CHECK_EQUAL(5, console.readRequest.recipient.portId);
}

TEST(Terminal, FailedOpenShowsNothingAndDoesNotRead)
{
    terminal.initialize();

    console.open(false);

    CHECK_OUTPUT("");
    CHECK_FALSE(console.reading);
    CHECK_EQUAL(0, bus.sent.size());
}

TEST(Terminal, TypedCharactersAreEchoed)
{
    start();

    console.type("ab");

    CHECK_OUTPUT("ab");
}

TEST(Terminal, SeveralKeysInOneReadAreAllHandled)
{
    start();

    console.deliver("abc");

    CHECK_OUTPUT("abc");
}

TEST(Terminal, NulPaddingAndControlCodesAreIgnored)
{
    start();

    console.deliver(etl::string_view("\0\0\x07", 3));

    CHECK_OUTPUT("");
}

TEST(Terminal, EnterOnEmptyLineShowsNewPrompt)
{
    start();

    console.type("\r");

    CHECK_OUTPUT("\r\nnpos:/> ");
}

TEST(Terminal, LineFeedAfterCarriageReturnIsOneEnter)
{
    start();

    console.deliver("\r\n");

    CHECK_OUTPUT("\r\nnpos:/> ");
}

TEST(Terminal, LineFeedAloneIsEnter)
{
    start();

    console.type("\n");

    CHECK_OUTPUT("\r\nnpos:/> ");
}

TEST(Terminal, UnknownCommandIsReported)
{
    start();

    console.type("xyz");
    console.type("\r");

    CHECK_OUTPUT("xyz\r\nxyz: command not found\r\nnpos:/> ");
}

TEST(Terminal, HelpListsBuiltinsAndPrograms)
{
    start();

    console.type("help\r");

    CHECK_OUTPUT("help\r\nhelp - list commands\r\nclear - clear screen\r\nfake - test program\r\nnpos:/> ");
}

TEST(Terminal, ClearSendsClearScreenSequence)
{
    start();

    console.type("clear\r");

    CHECK_OUTPUT("clear\r\n\x1b[2J\x1b[Hnpos:/> ");
}

TEST(Terminal, BackspaceAtEndOfLineRedrawsShorterLine)
{
    start();
    console.type("ab");
    console.take();

    console.type("\x7f");

    CHECK_OUTPUT(redraw("a").c_str());
}

TEST(Terminal, ControlHAlsoErases)
{
    start();
    console.type("a");
    console.take();

    console.type("\x08");

    CHECK_OUTPUT(redraw("").c_str());
}

TEST(Terminal, BackspaceOnEmptyLineDoesNothing)
{
    start();

    console.type("\x7f");

    CHECK_OUTPUT("");
}

TEST(Terminal, ErasedCharacterIsNotExecuted)
{
    start();
    console.type("hx\x7f");
    console.type("elp\r");
    console.take();

    console.type("help\r");

    CHECK_TRUE(strstr(console.take().c_str(), "command not found") == nullptr);
}

TEST(Terminal, LeftArrowMovesCursorBack)
{
    start();
    console.type("ab");
    console.take();

    console.type("\x1b[D");

    CHECK_OUTPUT("\x1b[1D");
}

TEST(Terminal, LeftArrowAtStartDoesNothing)
{
    start();

    console.type("\x1b[D");

    CHECK_OUTPUT("");
}

TEST(Terminal, RightArrowMovesCursorForward)
{
    start();
    console.type("ab\x1b[D\x1b[D");
    console.take();

    console.type("\x1b[C");

    CHECK_OUTPUT("\x1b[1C");
}

TEST(Terminal, RightArrowAtEndDoesNothing)
{
    start();
    console.type("ab");
    console.take();

    console.type("\x1b[C");

    CHECK_OUTPUT("");
}

TEST(Terminal, CharacterIsInsertedAtCursor)
{
    start();
    console.type("ac\x1b[D");
    console.take();

    console.type("b");

    CHECK_OUTPUT(redraw("abc", 1).c_str());
}

TEST(Terminal, BackspaceInMiddleRemovesCharacterBeforeCursor)
{
    start();
    console.type("abc\x1b[D");
    console.take();

    console.type("\x7f");

    CHECK_OUTPUT(redraw("ac", 1).c_str());
}

TEST(Terminal, DeleteRemovesCharacterUnderCursor)
{
    start();
    console.type("abc\x1b[H");
    console.take();

    console.type("\x1b[3~");

    CHECK_OUTPUT(redraw("bc", 2).c_str());
}

TEST(Terminal, DeleteAtEndDoesNothing)
{
    start();
    console.type("abc");
    console.take();

    console.type("\x1b[3~");

    CHECK_OUTPUT("");
}

TEST(Terminal, HomeAndEndMoveToLineBoundaries)
{
    start();
    console.type("abc");
    console.take();

    console.type("\x1b[H");
    CHECK_OUTPUT("\x1b[3D");

    console.type("\x1b[F");
    CHECK_OUTPUT("\x1b[3C");
}

TEST(Terminal, HomeAndEndHaveAlternativeEncodings)
{
    start();
    console.type("abc");
    console.take();

    console.type("\x1b[1~");
    CHECK_OUTPUT("\x1b[3D");
    console.type("\x1b[4~");
    CHECK_OUTPUT("\x1b[3C");
    console.type("\x1b[7~");
    CHECK_OUTPUT("\x1b[3D");
    console.type("\x1b[8~");
    CHECK_OUTPUT("\x1b[3C");
    console.type("\x1bOH");
    CHECK_OUTPUT("\x1b[3D");
    console.type("\x1bOF");
    CHECK_OUTPUT("\x1b[3C");
}

TEST(Terminal, ControlAAndEMoveToLineBoundaries)
{
    start();
    console.type("abc");
    console.take();

    console.type("\x01");
    CHECK_OUTPUT("\x1b[3D");

    console.type("\x05");
    CHECK_OUTPUT("\x1b[3C");
}

TEST(Terminal, ControlUClearsTheLine)
{
    start();
    console.type("abc");
    console.take();

    console.type("\x15");

    CHECK_OUTPUT(redraw("").c_str());
}

TEST(Terminal, ControlCDropsTheLineAndShowsNewPrompt)
{
    start();
    console.type("abc");
    console.take();

    console.type("\x03");
    CHECK_OUTPUT("^C\r\nnpos:/> ");

    console.type("\r");
    CHECK_OUTPUT("\r\nnpos:/> ");
}

TEST(Terminal, UnknownEscapeSequencesAreIgnored)
{
    start();
    console.type("ab");
    console.take();

    console.type("\x1b[Z");
    console.type("\x1bx");
    console.type("\x1b[99~");

    CHECK_OUTPUT("");
}

TEST(Terminal, SequenceModifiersAreSkipped)
{
    start();
    console.type("ab\x1b[H");
    console.take();

    console.type("\x1b[1;5C");

    CHECK_OUTPUT("\x1b[1C");
}

TEST(Terminal, LineLengthIsLimited)
{
    start();
    etl::string<200> keys(Terminal::LINE_SIZE + 10, 'x');

    console.type(etl::string_view(keys.c_str(), keys.size()));

    CHECK_EQUAL(Terminal::LINE_SIZE, console.take().size());
}

TEST(Terminal, UpArrowRecallsPreviousCommands)
{
    start();
    run("aa");
    run("bb");

    console.type("\x1b[A");
    CHECK_OUTPUT(redraw("bb").c_str());

    console.type("\x1b[A");
    CHECK_OUTPUT(redraw("aa").c_str());

    console.type("\x1b[A");
    CHECK_OUTPUT("");
}

TEST(Terminal, DownArrowMovesTowardsNewestAndRestoresTypedLine)
{
    start();
    run("aa");
    run("bb");
    console.type("dr");
    console.type("\x1b[A\x1b[A");
    console.take();

    console.type("\x1b[B");
    CHECK_OUTPUT(redraw("bb").c_str());

    console.type("\x1b[B");
    CHECK_OUTPUT(redraw("dr").c_str());

    console.type("\x1b[B");
    CHECK_OUTPUT("");
}

TEST(Terminal, HistoryHoldsFiveLines)
{
    start();
    run("c1");
    run("c2");
    run("c3");
    run("c4");
    run("c5");
    run("c6");

    for(auto expected : { "c6", "c5", "c4", "c3", "c2" })
    {
        console.type("\x1b[A");
        CHECK_OUTPUT(redraw(expected).c_str());
    }

    console.type("\x1b[A");
    CHECK_OUTPUT("");
}

TEST(Terminal, HistorySkipsEmptyAndRepeatedLines)
{
    start();
    run("aa");
    run("aa");
    run("");

    console.type("\x1b[A");
    CHECK_OUTPUT(redraw("aa").c_str());

    console.type("\x1b[A");
    CHECK_OUTPUT("");
}

TEST(Terminal, DownArrowWithoutHistoryNavigationDoesNothing)
{
    start();
    run("aa");

    console.type("\x1b[B");

    CHECK_OUTPUT("");
}

TEST(Terminal, RecalledLineCanBeEditedAndExecuted)
{
    start();
    console.type("hel");
    console.type("\x03");
    run("hel");
    console.type("\x1b[A");
    console.type("p");
    console.take();

    console.type("\r");

    CHECK_TRUE(strstr(console.take().c_str(), "list commands") != nullptr);
}

TEST(Terminal, RunsRegisteredProgramWithArguments)
{
    start();

    console.type("fake a  b\r");

    CHECK_EQUAL(1, program.started);
    CHECK_TRUE(program.args == "a  b");
    CHECK_OUTPUT("fake a  b\r\n");
}

TEST(Terminal, InputIsIgnoredWhileProgramRuns)
{
    start();
    console.type("fake\r");
    console.take();

    console.type("zz\x1b[A\r");

    CHECK_OUTPUT("");
}

TEST(Terminal, PromptReturnsWhenProgramExits)
{
    start();
    console.type("fake\r");
    console.take();

    program.finish();

    CHECK_OUTPUT("npos:/> ");

    console.type("a");
    CHECK_OUTPUT("a");
}

TEST(Terminal, ProgramThatExitsOnStartShowsPromptAtOnce)
{
    start();
    program.exitOnStart = true;

    console.type("fake\r");

    CHECK_OUTPUT("fake\r\nnpos:/> ");
}

TEST(Terminal, ExitOfProgramThatIsNotRunningIsIgnored)
{
    start();
    mock::FakeProgram other(api, "other");

    terminal.exit(other, 0);

    CHECK_OUTPUT("");
}

TEST(Terminal, RunningProgramIsResumedWhenOutputIsDrained)
{
    start();
    console.type("fake\r");
    console.take();
    auto resumedBefore = program.resumed;

    program.host->print("hi");
    console.pump();

    CHECK_EQUAL(resumedBefore + 1, program.resumed);
    CHECK_OUTPUT("hi");
}

TEST(Terminal, IdleTerminalDoesNotResumeAnything)
{
    start();

    console.type("a");

    CHECK_EQUAL(0, program.resumed);
}

TEST(Terminal, ResponsesAreForwardedToRunningProgram)
{
    start();
    console.type("fake\r");
    npos::ipc::Message response;
    response.type = npos::fs::msg::Type::FILE_OPEN;
    response.sender.portId = terminal.getId();

    console.feed(response);

    CHECK_EQUAL(1, program.handled.size());
    CHECK_EQUAL(npos::fs::msg::Type::FILE_OPEN, program.handled[0]);
}

TEST(Terminal, OpenAnswerGoesToTheRunningProgramOnceTheDeviceIsOpen)
{
    start();
    console.type("fake\r");
    console.take();
    npos::dev::msg::Open response("clock");
    response.setStatus(npos::dev::msg::Open::SUCCESS).setOpened({ 4, 1 });
    response.sender.portId = terminal.getId();

    console.feed(response);

    CHECK_EQUAL(1, program.handled.size());
    CHECK_EQUAL(npos::dev::msg::Type::OPEN, program.handled[0]);
    CHECK_OUTPUT("");
}

TEST(Terminal, ResponsesWithoutRunningProgramAreDropped)
{
    start();
    npos::ipc::Message response;
    response.type = npos::fs::msg::Type::FILE_OPEN;
    response.sender.portId = terminal.getId();

    console.feed(response);

    CHECK_EQUAL(0, program.handled.size());
}

TEST(Terminal, MessagesFromOtherPortsAreIgnored)
{
    terminal.initialize();
    npos::ipc::Message foreign = bus.sent[0];
    bus.sent.clear();
    foreign.sender.portId = 9;
    reinterpret_cast<npos::dev::msg::Open&>(foreign).setStatus(npos::dev::msg::Open::SUCCESS).setOpened({ 5, 0 });

    console.feed(foreign);

    CHECK_OUTPUT("");
    CHECK_FALSE(console.reading);
}

TEST(Terminal, AcceptsOnlyLogs)
{
    CHECK_TRUE(terminal.accepts(npos::cli::msg::Type::SYSLOG));
    CHECK_FALSE(terminal.accepts(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(terminal.accepts(npos::dev::msg::Type::TRANSCIEVER));
    CHECK_FALSE(terminal.accepts(npos::fs::msg::Type::FILE_OPEN));
    CHECK_FALSE(terminal.accepts(npos::dev::msg::Type::LED));
}

TEST(Terminal, RequestsOtherThanLogsAreReleasedWithoutQueueing)
{
    npos::ipc::Message request;
    request.type = npos::dev::msg::Type::OPEN;
    request.sender.portId = terminal.getId();

    terminal.receiveRequest(request);

    CHECK_FALSE(terminal.isReady());
    CHECK_EQUAL(1, bus.released.size());
    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, bus.responded[0].output.status);
}

TEST(Terminal, UnknownRequestHandlerAnswersNotSupported)
{
    npos::ipc::Message message;
    message.output.status = 0;

    terminal.handleUnknownRequest(message);

    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, bus.responded[0].output.status);
}

TEST(Terminal, LogReplacesPromptLineAndRestoresIt)
{
    start();
    console.type("ab");
    console.take();

    log("disk full");

    CHECK_OUTPUT("\r\x1b[K[{10:0} info] disk full\r\n\rnpos:/> ab\x1b[K");
}

TEST(Terminal, LogRestoresCursorPosition)
{
    start();
    console.type("ab\x1b[D");
    console.take();

    log("note");

    CHECK_OUTPUT("\r\x1b[K[{10:0} info] note\r\n\rnpos:/> ab\x1b[K\x1b[1D");
}

TEST(Terminal, LogOnEmptyPromptKeepsPrompt)
{
    start();

    log("boot");

    CHECK_OUTPUT("\r\x1b[K[{10:0} info] boot\r\n\rnpos:/> \x1b[K");
}

TEST(Terminal, LogWhileProgramRunsIsJustPrinted)
{
    start();
    console.type("fake\r");
    console.take();

    log("busy");

    CHECK_OUTPUT("[{10:0} info] busy\r\n");
}

TEST(Terminal, LogWithoutTextIsIgnored)
{
    start();
    npos::ipc::Message message;
    message.type = npos::cli::msg::Type::SYSLOG;
    message.sender.portId = 9;
    message.input.data = nullptr;

    console.feedRequest(message);

    CHECK_OUTPUT("");
}

TEST(Terminal, LogBeforeDeviceIsOpenedIsShownAfterTheGreeting)
{
    terminal.initialize();

    log("early");
    console.open();

    auto expected = greeting();
    expected.append("[{10:0} info] early\r\n");
    expected.append(PROMPT);
    CHECK_OUTPUT(expected.c_str());
}

TEST(Terminal, GreetingIsQueuedWhenTheTaskStartsAndSentOnceTheDeviceIsOpened)
{
    terminal.initialize();

    CHECK_EQUAL(0, console.writes);

    console.open();

    CHECK_TRUE(console.writes > 0);
    CHECK_OUTPUT(banner().c_str());
}

TEST(Terminal, GreetingContainsTheVersion)
{
    terminal.initialize();
    console.open();

    auto text = console.take();

    CHECK_TRUE(strstr(text.c_str(), "Welcome to npos ") != nullptr);
    CHECK_TRUE(strstr(text.c_str(), "Type 'help'") != nullptr);
}

TEST(Terminal, LogsAreHandledWhileOutputIsStillBeingSent)
{
    start();
    console.holdWrites = true;
    console.type("a");

    log("one");
    log("two");
    console.completeHeldWrite();

    auto text = console.take();
    CHECK_TRUE(strstr(text.c_str(), "[{10:0} info] one") != nullptr);
    CHECK_TRUE(strstr(text.c_str(), "[{10:0} info] two") != nullptr);
}

TEST(Terminal, PrintExpandsLineFeedsOnce)
{
    start();

    auto accepted = terminal.print("a\nb\r\nc");

    CHECK_EQUAL(6, accepted);
    CHECK_OUTPUT("a\r\nb\r\nc");
}

TEST(Terminal, PrintStopsWhenOutputBufferIsFull)
{
    start();
    console.holdWrites = true;
    etl::string<Terminal::OUTPUT_SIZE + 200> text(Terminal::OUTPUT_SIZE + 100, 'x');

    auto first = terminal.print(etl::string_view(text.c_str(), text.size()));
    console.pump();
    auto second = terminal.print("0123456789012345678901234567890123456789012345678901234567890123456789");

    CHECK_EQUAL(Terminal::OUTPUT_SIZE, first);
    CHECK_EQUAL(Terminal::CHUNK_SIZE, second);
}

TEST(Terminal, OutputIsSentInChunks)
{
    start();
    etl::string<300> text(200, 'x');
    auto writesBefore = console.writes;

    terminal.print(etl::string_view(text.c_str(), text.size()));
    console.pump();

    CHECK_EQUAL(4, console.writes - writesBefore);
    CHECK_EQUAL(200, console.take().size());
}

TEST(Terminal, WorkingDirectoryStartsAtRoot)
{
    CHECK_TRUE(terminal.getWorkingDirectory() == "/");
}

TEST(Terminal, WorkingDirectoryAppearsInPrompt)
{
    start();

    CHECK_TRUE(terminal.setWorkingDirectory("/data/logs"));
    console.type("\r");

    CHECK_OUTPUT("\r\nnpos:/data/logs> ");
    CHECK_TRUE(terminal.getWorkingDirectory() == "/data/logs");
}

TEST(Terminal, WorkingDirectoryMustBeAbsolute)
{
    CHECK_FALSE(terminal.setWorkingDirectory("relative"));
    CHECK_FALSE(terminal.setWorkingDirectory(""));
    CHECK_TRUE(terminal.getWorkingDirectory() == "/");
}

TEST(Terminal, WorkingDirectoryLengthIsLimited)
{
    etl::string<100> longest("/");
    longest.append(63, 'a');
    etl::string<100> tooLong("/");
    tooLong.append(64, 'a');

    CHECK_TRUE(terminal.setWorkingDirectory(etl::string_view(longest.c_str(), longest.size())));
    CHECK_FALSE(terminal.setWorkingDirectory(etl::string_view(tooLong.c_str(), tooLong.size())));
    CHECK_EQUAL(64, terminal.getWorkingDirectory().size());
}

TEST(Terminal, ProgramsSeeTheTerminalWorkingDirectory)
{
    start();
    terminal.setWorkingDirectory("/etc");
    console.type("fake\r");

    CHECK_TRUE(program.host->getWorkingDirectory() == "/etc");
}

TEST(Terminal, AddProgramFailsWhenListIsFull)
{
    mock::FakeProgram second(api, "two"), third(api, "three");

    CHECK_TRUE(terminal.addProgram(program));
    CHECK_TRUE(terminal.addProgram(second));
    CHECK_FALSE(terminal.addProgram(third));
}

TEST(Terminal, ReadingGivesUpAfterRepeatedErrors)
{
    start();

    console.failRead();
    CHECK_TRUE(console.reading);
    console.failRead();
    CHECK_TRUE(console.reading);
    console.failRead();

    CHECK_FALSE(console.reading);
}

TEST(Terminal, SuccessfulReadResetsErrorCount)
{
    start();

    console.failRead();
    console.failRead();
    console.type("a");
    console.failRead();
    console.failRead();

    CHECK_TRUE(console.reading);
}

TEST(Terminal, FailedWriteDoesNotStallOutput)
{
    start();
    console.holdWrites = true;
    console.type("a");
    auto& request = reinterpret_cast<npos::dev::msg::Transciever&>(console.heldWrite);
    request.setStatus(npos::dev::msg::Transciever::DEVICE_ERROR);
    console.holdWrites = false;
    console.feed(console.heldWrite);
    console.pump();
    console.take();

    console.type("b");

    CHECK_OUTPUT("b");
}

TEST(Terminal, KeepsPriority)
{
    CHECK_EQUAL(2, terminal.getPriority());
}

TEST(Terminal, IsReadyOnlyWithQueuedMessages)
{
    npos::ipc::Message message;
    message.type = npos::dev::msg::Type::OPEN;
    message.sender.portId = terminal.getId();

    CHECK_FALSE(terminal.isReady());

    terminal.receiveResponse(message);

    CHECK_TRUE(terminal.isReady());

    terminal.process();

    CHECK_FALSE(terminal.isReady());
}

TEST(Terminal, ProcessWithoutMessagesDoesNothing)
{
    terminal.process();

    CHECK_EQUAL(0, bus.released.size());
}

TEST(Terminal, ProcessedMessagesAreReleasedToTheBus)
{
    start();
    npos::ipc::Message response;
    response.type = npos::fs::msg::Type::FILE_OPEN;
    response.sender.portId = terminal.getId();
    auto before = bus.released.size();

    console.feed(response);

    CHECK_EQUAL(before + 1, bus.released.size());
    POINTERS_EQUAL(&response, bus.released.back());
}

TEST(Terminal, MessagesFromOtherPortsAreReleasedWithoutQueueing)
{
    npos::ipc::Message foreign;
    foreign.type = npos::dev::msg::Type::OPEN;
    foreign.sender.portId = 9;

    terminal.receiveResponse(foreign);

    CHECK_FALSE(terminal.isReady());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(Terminal, LogTextIsCopiedToThePayloadPoolOnReceive)
{
    start();
    char text[] = "original";
    npos::cli::msg::Syslog message({ 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, text, false);
    message.sender.portId = 10;

    terminal.receiveRequest(message);
    text[0] = 'X';
    terminal.process();
    console.pump();

    CHECK_TRUE(strstr(console.take().c_str(), "original") != nullptr);
}

TEST(Terminal, QueuedLogPointsAtAPayloadSlot)
{
    npos::cli::msg::Syslog message({ 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, "abc", false);

    terminal.receiveRequest(message);

    CHECK_TRUE(payloads.is_in_pool(message.input.data));
    CHECK_EQUAL(3, payloads.available());
}

TEST(Terminal, PayloadSlotIsFreedAfterTheLogIsHandled)
{
    start();

    log("one");
    log("two");

    CHECK_EQUAL(4, payloads.available());
}

TEST(Terminal, LogsAreDroppedWhenPayloadPoolIsFull)
{
    npos::cli::msg::Syslog logs[5] = {
        { { 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, "a", false }, { { 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, "b", false },
        { { 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, "c", false }, { { 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, "d", false },
        { { 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, "e", false } };

    for(auto& message : logs)
        terminal.receiveRequest(message);

    CHECK_EQUAL(0, payloads.available());
    CHECK_EQUAL(1, bus.released.size());
    POINTERS_EQUAL(&logs[4], bus.released[0]);
}

TEST(Terminal, TooLongLogIsDropped)
{
    etl::string<200> text(npos::cli::utils::LogStream::MAX_MESSAGE_LENGTH + 1, 'x');
    npos::cli::msg::Syslog message({ 10, 0 }, { terminal.getId(), 0 }, npos::cli::log::Level::INFO, etl::string_view(text.c_str(), text.size()), false);

    terminal.receiveRequest(message);

    CHECK_FALSE(terminal.isReady());
    CHECK_EQUAL(4, payloads.available());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(Terminal, LogIsDroppedAndSlotFreedWhenQueueIsFull)
{
    Terminal::MessageQueue<1> smallQueue;
    Terminal small{2, smallQueue, payloads, programList, bus, "uart0"};
    npos::cli::msg::Syslog first({ 10, 0 }, { small.getId(), 0 }, npos::cli::log::Level::INFO, "a", false);
    npos::cli::msg::Syslog second({ 10, 0 }, { small.getId(), 0 }, npos::cli::log::Level::INFO, "b", false);

    small.receiveRequest(first);
    small.receiveRequest(second);

    CHECK_EQUAL(3, payloads.available());
    CHECK_EQUAL(1, bus.released.size());
    POINTERS_EQUAL(&second, bus.released[0]);
}

TEST(Terminal, RequestsAreSentFromTheTerminalPort)
{
    terminal.initialize();

    CHECK_EQUAL(3, bus.sent[0].sender.portId);
}

TEST(Terminal, ActsAsApiOfItsPrograms)
{
    npos::ipc::Service::Api& terminalApi = terminal;
    mock::FakeProgram owner{terminal};
    owner.setId(4);
    npos::ipc::Message message;
    message.type = npos::fs::msg::Type::FILE_OPEN;

    terminalApi.send(owner, message);
    terminalApi.respond(message);

    CHECK_EQUAL(3, terminalApi.getPortId());
    CHECK_EQUAL(3, bus.sent[0].sender.portId);
    CHECK_EQUAL(4, bus.sent[0].sender.serviceId);
    CHECK_EQUAL(1, bus.responded.size());
}
