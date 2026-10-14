# npos

A small cooperative operating system framework for microcontrollers, written in C++17 with
[ETL](https://www.etlcpp.com/) (no dynamic allocation). Hardware is reached through HAL interfaces,
which are replaced by an emulator when running on a PC.

Full documentation (Polish) is in [docs](docs): build it with `make -C docs`,
the result is `docs/manual.pdf`. Sources are the `*.roff` chapters in [docs/source](docs/source).

## Libraries

Three libraries are built by the top level `Makefile`.

| Library | Purpose |
|---|---|
| `npos` | The core, header only, in `npos/libnpos/<module>` (see modules below) |
| `npos-emulator` | HAL implementations for running on a PC |
| `npos-os` | `os::System` builds a whole system from HAL interfaces |

Modules of `npos`, included as `<libnpos/<module>/File.hpp>`:

| Module | Purpose |
|---|---|
| `nps` | Cooperative scheduler, tasks, idle and watchdog hooks |
| `ipc` | Messages, `Oid` addressing, services, `ServiceTask`, `SystemBus`, `FsmService` |
| `dev` | Device services (LED, RTC, backup registers, UART polling/interrupt, hash), timer, idle, watchdog |
| `fs` | File and directory messages (including `mkdir`) and the littlefs based `Littlefs` task |
| `cli` | Terminal task (line editing, history, logs), programs, logging (`cli::api::syslog`) |

## Concepts

- A **task** is run by the scheduler; a `ServiceTask` is a task, a bus port and the `Service::Api` of its services.
- A **service** handles `ipc::Message`s. Objects that can fail (open devices, files, directories) are addressed by `Oid` = (port, service).
- Requests to an `Oid` go straight to the owner; only opening a device by name is a broadcast.
- Containers, queues and pools are never allocated by the library, they are always provided by the user.

## Building and testing

```sh
make            # builds libraries, runs all unit tests (CppUTest), builds the examples
```

Unit tests of the core live in `npos/tests/<module>/` as `<Module>.test.cpp` and form one test program;
`npos-emulator` and `npos-os` have their own `tests`. Mocks are in `tests/mock`. Test file names must be unique
in the whole library, so a letter-suffix is added on a clash (e.g. `ApiDev.test.cpp`, `ApiFs.test.cpp`).
New test directories must be listed in `npos/tests/Makefile`. A failing test fails the build.
Output of the emulator is silent unless `verbose` is passed to a mock (the examples do that).

## Building a system

```cpp
npos::emulator::SystemTick tick;
npos::emulator::Rtc rtc;
npos::os::Device<npos::dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };

npos::os::Storage<> storage;                    // all containers, queues and pools (injected, never allocated by the library)
npos::os::UserApi api;                          // lets user services exist before the system
MyService service(api);
npos::ipc::Service* services[] = { &service };

npos::os::Hal hal(tick);                        // system tick is mandatory
hal.rtcs = rtcs;                                // also: flash/icache, power, watchdog, leds,
                                                // backups, pollingUarts, uarts, hashes

npos::os::Config config;                        // task priorities (with defaults), cli flag
npos::os::System system(hal, storage.memory(), services, api, config);
system.start();
```

`System` creates the user, device, interrupt device, filesystem, timer and (with `cli`) terminal tasks, and the idle and watchdog tasks
when `power` and `watchdog` are given. Any number of devices of each kind can be added, each under its own name.

## Logging

Logging belongs to the `cli` module, the `ipc` module has no knowledge of it:

```cpp
cli::api::syslog(*this, cli::log::WARNING) << "voltage " << 12 << cli::log::endl;
```

`cli::utils::LogStream` collects up to 128 characters and `cli::log::endl` broadcasts a `cli::msg::Syslog`
message (level, sender `Oid`, text) to every port that accepts it, i.e. the terminal. A log without `endl` is not sent.

## CLI

With `config.cli = true` the terminal runs on a transciever device (`config.terminalDevice`). It supports
line editing (arrows, Home, End, Delete, Ctrl-A/E/U/C), a 5 line history and prints logs above the edited line
(`Terminal::setLogLevel()` sets the minimum level, INFO by default). The terminal is a task of its own and the `Api` of its programs;
the log text is copied into a pool slot on receive, because the sender's buffer exists only during the call.
Programs: `cat`, `echo` (with `>` and `>>`), `touch`, `ls`, `cd`, `pwd`, `mkdir`. Paths are at most 64 characters.

Try it: `./bin/Pc32/debug/cli.elf` (input comes from stdin, so pace piped commands with short pauses).

## Examples

`examples/` contains `blinking-led`, `get-time`, `compute-hash`, `uart-read-write`, `filesystem`, `dual-system` and `cli`.
