# npos

A small cooperative operating system framework for microcontrollers, written in C++17 with
[ETL](https://www.etlcpp.com/) (no dynamic allocation). Hardware is reached through HAL interfaces,
which are replaced by an emulator when running on a PC.

Full documentation (Polish) is in [docs/technical](docs/technical): build it with `make -C docs/technical`,
the result is `docs/technical/manual.pdf`. Sources are the `*.roff` chapters in
[docs/technical/source](docs/technical/source).

## Libraries

| Library | Purpose |
|---|---|
| `npos-nps` | Cooperative scheduler, tasks, idle and watchdog hooks |
| `npos-ipc` | Messages, `Oid` addressing, services, `ServiceTask`, `SystemBus`, `FsmService` |
| `npos-dev` | Device services (LED, RTC, backup registers, UART polling/interrupt, hash), timer, idle, watchdog |
| `npos-fs` | File and directory messages and the littlefs based `Littlefs` task |
| `npos-cli` | Terminal task (line editing, history, logs) and programs |
| `npos-os` | `os::System` builds a whole system from HAL interfaces |
| `npos-emulator` | HAL implementations for running on a PC |

## Concepts

- A **task** is run by the scheduler; a `ServiceTask` is a task, a bus port and the `Service::Api` of its services.
- A **service** handles `ipc::Message`s. Objects that can fail (open devices, files, directories) are addressed by `Oid` = (port, service).
- Requests to an `Oid` go straight to the owner; only opening a device by name is a broadcast.
- Containers, queues and pools are never allocated by the library, they are always provided by the user.

## Building and testing

```sh
make            # builds libraries, runs all unit tests (CppUTest), builds the examples
```

Unit tests live in `<library>/tests` as `<Module>.test.cpp`, mocks in `tests/mock`. A failing test fails the build.
Output of the emulator is silent unless `verbose` is passed to a mock (the examples do that).

## Building a system

```cpp
npos::emulator::SystemTick tick;
npos::emulator::Rtc rtc;
npos::os::Device<npos::dev::hal::Rtc> rtcs[] = { { rtc, "clock" } };

npos::os::Storage<> storage;                    // all containers, queues and pools
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

`System` creates the user, device, interrupt device, filesystem and timer tasks, and the idle and watchdog tasks
when `power` and `watchdog` are given. Any number of devices of each kind can be added, each under its own name.

## CLI

With `config.cli = true` the terminal runs on a transciever device (`config.terminalDevice`). It supports
line editing (arrows, Home, End, Delete, Ctrl-A/E/U/C), a 5 line history and prints system logs above the edited line.
Programs: `cat`, `echo` (with `>` and `>>`), `touch`, `ls`, `cd`, `pwd`, `mkdir`. Paths are at most 64 characters.

Try it: `./bin/Pc32/debug/cli.elf` (input comes from stdin, so pace piped commands with short pauses).

## Examples

`examples/` contains `blinking-led`, `get-time`, `compute-hash`, `uart-read-write`, `filesystem`, `dual-system` and `cli`.
