#pragma once

/**
 * @file Terminal.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstring>

#include <etl/algorithm.h>
#include <etl/pool.h>
#include <etl/queue.h>
#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/to_string.h>
#include <etl/vector.h>

#include <libnpos/Version.hpp>
#include <libnpos/nps/Task.hpp>
#include <libnpos/ipc/Bus.hpp>
#include <libnpos/ipc/Port.hpp>
#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Transciever.hpp>
#include <libnpos/cli/msg/Syslog.hpp>
#include <libnpos/fs/msg/Type.hpp>

namespace npos::cli::task
{
    // Line terminal over a dev::msg::Transciever device (e.g. uart). Runs registered programs.
    // It is a task of its own and the Api of its programs, so it gets every answer addressed to it.
    class Terminal : public nps::Task, public ipc::Port, public ipc::Service::Api, public Program::Host
    {
    public:
        static constexpr size_t LINE_SIZE = 128;
        static constexpr size_t READ_SIZE = 64;
        static constexpr size_t OUTPUT_SIZE = 1024;
        static constexpr size_t CHUNK_SIZE = 64;
        static constexpr size_t MAX_READ_ERRORS = 3;
        static constexpr size_t HISTORY_SIZE = 5;
        static constexpr size_t GREETING_SIZE = 96;

        using MessageQueueInt = etl::iqueue<ipc::Message*>;
        using PayloadPoolInt = etl::ipool;
        using ProgramListInt = etl::ivector<Program*>;

        template<size_t N>
        using MessageQueue = etl::queue<ipc::Message*, N>;

        template<size_t N>
        using ProgramList = etl::vector<Program*, N>;

        // Slots of the payload pool hold the text of one log each.
        Terminal(Priority priority, MessageQueueInt& messageQueue, PayloadPoolInt& payloads, ProgramListInt& programs,
            ipc::Bus& bus, etl::string_view deviceName)
            : nps::Task(priority)
            , messageQueue(messageQueue)
            , payloads(payloads)
            , programs(programs)
            , bus(bus)
            , deviceName(deviceName)
            , logLevel(log::Level::INFO)
        {
        }

        bool addProgram(Program& program)
        {
            if(programs.full())
                return false;

            programs.push_back(&program);
            return true;
        }

        void setLogLevel(log::Level level)
        {
            logLevel = level;
        }

        //Port
        bool accepts(ipc::Message::Type type) const override
        {
            return type == cli::msg::SYSLOG;
        }

        // The log text lives in the sender's buffer, so it is moved to a slot of the payload pool before queueing.
        void receiveRequest(const ipc::Message& message) override
        {
            auto& request = const_cast<ipc::Message&>(message);

            if(request.type != cli::msg::SYSLOG)
            {
                handleUnknownRequest(request);
                return bus.release(&request);
            }

            if(not storeLog(request))
                return bus.release(&request);

            if(messageQueue.full())
            {
                payloads.release(request.input.data);
                return bus.release(&request);
            }

            messageQueue.push(&request);
        }

        // Only answers to our own requests matter.
        void receiveResponse(const ipc::Message& message) override
        {
            auto& response = const_cast<ipc::Message&>(message);

            if(response.sender.portId != getId() or messageQueue.full())
                return bus.release(&response);

            messageQueue.push(&response);
        }

        void handleUnknownRequest(ipc::Message& message) override
        {
            message.output.status = ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED;
            bus.respond(message);
        }

        //nps::Task
        void initialize() override
        {
            // Kept in the output buffer until the device is opened.
            printGreeting();

            dev::msg::Open request(deviceName);
            request.sender.portId = getId();
            bus.send(request);
        }

        bool isReady() const override
        {
            return not messageQueue.empty();
        }

        void process() override
        {
            if(messageQueue.empty())
                return;

            auto message = messageQueue.front();
            handle(*message);

            if(message->type == cli::msg::SYSLOG)
                payloads.release(message->input.data);

            messageQueue.pop();
            bus.release(message);
        }

        //Service::Api
        void send(ipc::Service& sender, ipc::Message& message) override
        {
            message.sender = sender.getOid();
            bus.send(message);
        }

        void respond(ipc::Message& message) override
        {
            bus.respond(message);
        }

        ipc::PortId getPortId() const override
        {
            return getId();
        }

        // Program::Host
        size_t print(etl::string_view output) override
        {
            size_t accepted = 0;

            for(auto c : output)
            {
                bool expand = (c == '\n' and lastOutput != '\r');

                if(pending.available() < (expand ? 2u : 1u))
                    break;

                if(expand)
                    pending.push_back('\r');

                pending.push_back(static_cast<uint8_t>(c));
                lastOutput = c;
                accepted++;
            }

            flush();
            return accepted;
        }

        void exit(Program& program, int) override
        {
            if(running != &program)
                return;

            running = nullptr;
            prompt();
        }

        etl::string_view getWorkingDirectory() const override
        {
            return etl::string_view(cwd.c_str(), cwd.size());
        }

        bool setWorkingDirectory(etl::string_view path) override
        {
            if(path.empty() or path.front() != '/' or path.size() > utils::text::MAX_PATH)
                return false;

            cwd.assign(path.data(), path.size());
            return true;
        }

    private:
        enum class Escape { NONE, ESC, CSI, SS3 };

        bool storeLog(ipc::Message& message)
        {
            if(not message.input.data or message.input.size > payloads.max_item_size() or payloads.full())
                return false;

            auto slot = payloads.allocate<uint8_t>();
            std::memcpy(slot, message.input.data, message.input.size);
            message.input.data = slot;
            return true;
        }

        // SYSLOG is the only request the terminal serves, everything else queued is an answer.
        // Once the terminal device is open, an Open answer belongs to the running program.
        void handle(ipc::Message& message)
        {
            if(message.type == cli::msg::Type::SYSLOG)
                onLog(message);
            else if(message.type == dev::msg::Type::OPEN and not opened)
                onOpen(static_cast<dev::msg::Open&>(message));
            else if(message.type == dev::msg::Type::TRANSCIEVER)
                onTransciever(static_cast<dev::msg::Transciever&>(message));
            else if(running)
                running->handleResponse(message);
        }

        void onOpen(dev::msg::Open& response)
        {
            if(response.getStatus() != dev::msg::Open::Status::SUCCESS)
                return;

            device = response.getOpened();
            opened = true;

            prompt();
            startRead();
        }

        void printGreeting()
        {
            etl::string<GREETING_SIZE> text("\r\nWelcome to npos ");

            etl::to_string(static_cast<unsigned>(VERSION.major), text, true);
            text.push_back('.');
            etl::to_string(static_cast<unsigned>(VERSION.minor), text, true);
            text.push_back('.');
            etl::to_string(static_cast<unsigned>(VERSION.patch), text, true);
            text.append("\r\nType 'help' to list the available programs.\r\n");

            print(etl::string_view(text.data(), text.size()));
        }

        void onTransciever(dev::msg::Transciever& response)
        {
            bool success = response.getStatus() == dev::msg::Transciever::SUCCESS;

            if(response.getOperation() == dev::msg::Transciever::READ)
            {
                if(success)
                {
                    readErrors = 0;
                    for(size_t i = 0; i < response.getOutputLength(); i++)
                        onByte(rxBuffer[i]);
                    startRead();
                }
                else if(++readErrors < MAX_READ_ERRORS)
                    startRead();
            }
            else
            {
                writing = false;
                flush();

                if(not writing and running)
                    running->resume();
            }
        }

        // Prints the log above the edited line, then restores the line.
        void onLog(ipc::Message& message)
        {
            auto& request = static_cast<cli::msg::Syslog&>(message);

            if(not message.input.data)
                return;

            if(request.getLevel() < logLevel)
                return;

            if(promptShown)
                print("\r\x1b[K");

            print("[");
            print(utils::text::printOid(request.getSender()));
            print(" ");
            print(utils::text::syslogLevel(request.getLevel()));
            if(request.getOverflow())
                print("!!!");
            print("] ");
            print(request.getMessage());
            print("\r\n");

            if(promptShown)
                redraw();
        }

        // Every received byte is a key press.
        void onByte(uint8_t c)
        {
            if(running)
            {
                escape = Escape::NONE;
                return;
            }

            switch(escape)
            {
            case Escape::ESC:
                if(c == '[')
                {
                    escape = Escape::CSI;
                    param = 0;
                    afterSeparator = false;
                }
                else
                    escape = (c == 'O') ? Escape::SS3 : Escape::NONE;
                return;

            case Escape::SS3:
                escape = Escape::NONE;
                return onSequence(c, 0);

            case Escape::CSI:
                if(c >= '0' and c <= '9')
                {
                    if(not afterSeparator)
                        param = param * 10 + (c - '0');
                }
                else if(c == ';')
                    afterSeparator = true;
                else if(c >= 0x40 and c <= 0x7e)
                {
                    escape = Escape::NONE;
                    onSequence(c, param);
                }
                return;

            case Escape::NONE:
                break;
            }

            bool skipLineFeed = (c == '\n' and lastInput == '\r');
            lastInput = c;

            if(skipLineFeed)
                return;

            switch(c)
            {
            case 0x1b: escape = Escape::ESC; break;
            case '\r':
            case '\n': enter(); break;
            case 0x7f:
            case 0x08: backspace(); break;
            case 0x01: moveTo(0); break;
            case 0x05: moveTo(line.size()); break;
            case 0x15: replaceLine(etl::string_view()); break;
            case 0x03:
                print("^C\r\n");
                prompt();
                break;
            default:
                if(c >= 0x20 and c < 0x7f)
                    insert(static_cast<char>(c));
                break;
            }
        }

        void onSequence(uint8_t final, size_t number)
        {
            switch(final)
            {
            case 'A': return historyPrevious();
            case 'B': return historyNext();
            case 'C': return moveTo(cursor + 1);
            case 'D': return moveTo(cursor > 0 ? cursor - 1 : 0);
            case 'H': return moveTo(0);
            case 'F': return moveTo(line.size());
            case '~':
                if(number == 1 or number == 7)
                    moveTo(0);
                else if(number == 4 or number == 8)
                    moveTo(line.size());
                else if(number == 3)
                    erase();
                return;
            default:
                return;
            }
        }

        void insert(char c)
        {
            if(line.full())
                return;

            if(cursor == line.size())
            {
                line.push_back(c);
                cursor++;
                print(etl::string_view(&c, 1));
            }
            else
            {
                line.insert(line.begin() + cursor, c);
                cursor++;
                redraw();
            }

            historyIndex = -1;
        }

        void backspace()
        {
            if(cursor == 0)
                return;

            cursor--;
            erase();
        }

        // Deletes the character under the cursor.
        void erase()
        {
            if(cursor >= line.size())
                return;

            line.erase(line.begin() + cursor);
            historyIndex = -1;
            redraw();
        }

        void moveTo(size_t position)
        {
            if(position > line.size())
                position = line.size();

            if(position < cursor)
                move(cursor - position, 'D');
            else if(position > cursor)
                move(position - cursor, 'C');

            cursor = position;
        }

        void move(size_t count, char direction)
        {
            etl::string<12> sequence("\x1b[");
            char digits[8];
            size_t length = 0;

            for(; count > 0 and length < sizeof(digits); count /= 10)
                digits[length++] = static_cast<char>('0' + count % 10);

            while(length > 0)
                sequence.push_back(digits[--length]);

            sequence.push_back(direction);
            print(etl::string_view(sequence.c_str(), sequence.size()));
        }

        void redraw()
        {
            print("\r");
            printPrompt();
            print(etl::string_view(line.c_str(), line.size()));
            print("\x1b[K");

            if(cursor < line.size())
                move(line.size() - cursor, 'D');
        }

        void replaceLine(etl::string_view text)
        {
            line.assign(text.data(), text.size());
            cursor = line.size();
            redraw();
        }

        void historyPrevious()
        {
            if(historyIndex + 1 >= static_cast<int>(historyCount))
                return;

            if(historyIndex < 0)
                draft = line;

            historyIndex++;
            replaceLine(etl::string_view(history[historyIndex].c_str(), history[historyIndex].size()));
        }

        void historyNext()
        {
            if(historyIndex < 0)
                return;

            historyIndex--;

            if(historyIndex < 0)
                replaceLine(etl::string_view(draft.c_str(), draft.size()));
            else
                replaceLine(etl::string_view(history[historyIndex].c_str(), history[historyIndex].size()));
        }

        void remember()
        {
            if(line.empty() or (historyCount > 0 and history[0] == line))
                return;

            for(size_t i = HISTORY_SIZE - 1; i > 0; i--)
                history[i] = history[i - 1];

            history[0] = line;

            if(historyCount < HISTORY_SIZE)
                historyCount++;
        }

        void enter()
        {
            print("\r\n");
            promptShown = false;
            remember();
            historyIndex = -1;
            execute();
        }

        void execute()
        {
            auto rest = etl::string_view(line.c_str(), line.size());
            auto command = utils::text::token(rest);

            if(command.empty())
                return prompt();

            if(command == "help")
                return help();

            if(command == "clear")
            {
                print("\x1b[2J\x1b[H");
                return prompt();
            }

            for(auto program : programs)
            {
                if(program->getName() == command)
                {
                    running = program;
                    return program->start(*this, rest);
                }
            }

            print(command);
            print(": command not found\r\n");
            prompt();
        }

        void help()
        {
            print("help - list commands\r\nclear - clear screen\r\n");

            for(auto program : programs)
            {
                print(program->getDescription());
                print("\r\n");
            }

            prompt();
        }

        void prompt()
        {
            line.clear();
            cursor = 0;
            promptShown = true;
            printPrompt();
        }

        void printPrompt()
        {
            print("npos:");
            print(etl::string_view(cwd.c_str(), cwd.size()));
            print("> ");
        }

        void startRead()
        {
            dev::msg::Transciever request(device, dev::msg::Transciever::READ, READ_SIZE, etl::span<uint8_t>(rxBuffer, READ_SIZE));
            request.sender.portId = getId();
            bus.send(request);
        }

        void flush()
        {
            if(not opened or writing or pending.empty())
                return;

            auto size = pending.size() < CHUNK_SIZE ? pending.size() : CHUNK_SIZE;
            etl::copy_n(pending.begin(), size, chunk);
            pending.erase(pending.begin(), pending.begin() + size);

            writing = true;
            dev::msg::Transciever request(device, dev::msg::Transciever::WRITE, size, etl::span<uint8_t>(chunk, size));
            request.sender.portId = getId();
            bus.send(request);
        }

        MessageQueueInt& messageQueue;
        PayloadPoolInt& payloads;
        ProgramListInt& programs;
        ipc::Bus& bus;
        etl::string_view deviceName;
        ipc::Oid device = ipc::NULL_OID;
        log::Level logLevel;

        bool opened = false;
        bool writing = false;
        size_t readErrors = 0;
        Program* running = nullptr;

        etl::string<utils::text::MAX_PATH> cwd = "/";
        etl::string<LINE_SIZE> line;
        size_t cursor = 0;
        bool promptShown = false;

        etl::string<LINE_SIZE> history[HISTORY_SIZE];
        size_t historyCount = 0;
        int historyIndex = -1;
        etl::string<LINE_SIZE> draft;

        Escape escape = Escape::NONE;
        size_t param = 0;
        bool afterSeparator = false;

        etl::vector<uint8_t, OUTPUT_SIZE> pending;
        uint8_t chunk[CHUNK_SIZE];
        uint8_t rxBuffer[READ_SIZE];
        uint8_t lastInput = 0;
        char lastOutput = 0;
    };
}
