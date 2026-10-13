#pragma once

/**
 * @file Terminal.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/algorithm.h>
#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/Text.hpp>
#include <libnpos/dev/Api.hpp>
#include <libnpos/fs/msg/Type.hpp>

namespace npos::cli::service
{
    // Line terminal over a dev::msg::Transciever device (e.g. uart). Runs registered programs.
    // Subscribe it first in its task: ipc routes responses by recipient service id, which is 0 for the device and the first file.
    class Terminal : public ipc::Service, public Program::Host
    {
    public:
        static constexpr size_t LINE_SIZE = 128;
        static constexpr size_t READ_SIZE = 64;
        static constexpr size_t OUTPUT_SIZE = 1024;
        static constexpr size_t CHUNK_SIZE = 64;
        static constexpr size_t MAX_READ_ERRORS = 3;
        static constexpr size_t HISTORY_SIZE = 5;

        using ProgramListInt = etl::ivector<Program*>;

        template<size_t N>
        using ProgramList = etl::vector<Program*, N>;

        Terminal(Api& api, ProgramListInt& programs, etl::string_view deviceName)
            : ipc::Service(api)
            , programs(programs)
            , deviceName(deviceName)
        {
        }

        // Programs are driven by the terminal, they must not be subscribed to the task.
        bool addProgram(Program& program)
        {
            if(programs.full())
                return false;

            programs.push_back(&program);
            return true;
        }

        void initialize() override
        {
            dev::api::open(*this, deviceName);
        }

        void handle(ipc::Message& message) override
        {
            if(message.type == ipc::OS_SYSLOG_MESSAGE_TYPE)
                return onLog(message);

            // Others are broadcast to every accepting port, only answers to our own requests matter.
            if(message.sender != api.getPortId())
                return;

            if(message.type == dev::msg::Type::OPEN)
                onOpen(static_cast<dev::msg::Open&>(message));
            else if(message.type == dev::msg::Type::TRANSCIEVER)
                onTransciever(static_cast<dev::msg::Transciever&>(message));
            else if(running)
                running->handle(message);
        }

        bool accepts(ipc::Message::Type type) const override
        {
            switch(type)
            {
            case ipc::OS_SYSLOG_MESSAGE_TYPE:
            case dev::msg::Type::OPEN:
            case dev::msg::Type::TRANSCIEVER:
            case fs::msg::Type::FILE_OPEN:
            case fs::msg::Type::FILE_CLOSE:
            case fs::msg::Type::FILE_READ:
            case fs::msg::Type::FILE_WRITE:
            case fs::msg::Type::DIR_OPEN:
            case fs::msg::Type::DIR_CLOSE:
            case fs::msg::Type::DIR_READ:
                return true;
            default:
                return false;
            }
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
            if(path.empty() or path.front() != '/' or path.size() > text::MAX_PATH)
                return false;

            cwd.assign(path.data(), path.size());
            return true;
        }

    private:
        enum class Escape { NONE, ESC, CSI, SS3 };

        void onOpen(dev::msg::Open& response)
        {
            if(response.getStatus() != dev::msg::Open::Status::SUCCESS)
                return;

            device = response.getOpened();
            opened = true;

            print("\r\nnpos terminal, type 'help'\r\n");
            prompt();
            startRead();
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
            if(not message.input.data)
                return;

            if(promptShown)
                print("\r\x1b[K");

            print(etl::string_view(reinterpret_cast<const char*>(message.input.data), message.input.size));
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
            auto command = text::token(rest);

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
            dev::api::transcieverRead(*this, device, READ_SIZE, etl::span<uint8_t>(rxBuffer, READ_SIZE));
        }

        void flush()
        {
            if(not opened or writing or pending.empty())
                return;

            auto size = pending.size() < CHUNK_SIZE ? pending.size() : CHUNK_SIZE;
            etl::copy_n(pending.begin(), size, chunk);
            pending.erase(pending.begin(), pending.begin() + size);

            writing = true;
            dev::api::transcieverWrite(*this, device, etl::span<const uint8_t>(chunk, size));
        }

        ProgramListInt& programs;
        etl::string_view deviceName;
        ipc::Oid device = ipc::NULL_OID;
        bool opened = false;
        bool writing = false;
        size_t readErrors = 0;
        Program* running = nullptr;

        etl::string<text::MAX_PATH> cwd = "/";
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
