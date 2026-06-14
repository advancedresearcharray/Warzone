# Warzone

Console-focused matchmaking logic for Call of Duty: Warzone, implemented in **C++** for console-native deployment (Xbox GDK, PlayStation SDK, or similar).

## The idea

Warzone supports cross-play between PC, Xbox, and PlayStation. That is great for population size, but many console players prefer **not** to match against PC — whether because of input differences, perceived advantage, or cheat concerns.

What console players often *do* want is **cross-play between consoles**: Xbox and PlayStation in the same lobby, without PC players mixed in.

This project is a **matchmaking rules module** that encodes that preference:

- **Block PC** players from console-only pools
- **Allow Xbox ↔ PlayStation** cross-play
- **Validate parties** so mixed console squads can queue together
- **Form lobbies and squads** using region and optional skill-band rules

It is a standalone C++ library and CLI. It does **not** hook into Activision's live matchmaking servers, but it is written in a form suitable for integration with console toolchains and backends.

## How matching works

Each player has a **platform** and a **cross-play preference**:

| Platform | Examples |
|----------|----------|
| PC | Windows / Battle.net / Steam |
| Xbox | Series X\|S, Xbox One |
| PlayStation | PS5, PS4 |

| Preference | Behavior |
|------------|----------|
| `console_only` | Xbox + PlayStation allowed; PC blocked |
| `same_platform` | Only match within the same family (e.g. PS with PS) |
| `all` | Match with any platform (default Warzone behavior) |

Under `console_only` — the main use case for this repo:

```
✅ Xbox Series  +  PS5
✅ Xbox One     +  PS4
✅ Mixed console party (Xbox + PlayStation friends)
❌ Any console  +  PC
❌ PC player entering the console-only queue
```

Both sides must agree: if a console player wants `console_only`, a PC player cannot join their lobby even if the PC player has cross-play enabled.

## Project structure

```
include/warzone/
  types.hpp       Player, party, and config types
  platform.hpp    Platform detection and compatibility rules
  matcher.hpp     Pool filtering, lobby/squad formation, party validation
  queue.hpp       In-memory matchmaking queue
src/
  types.cpp
  platform.cpp
  matcher.cpp
  queue.cpp
  cli/main.cpp    Command-line tool
tests/
  matcher_test.cpp
CMakeLists.txt
```

## Quick start

Requires CMake 3.16+, a C++20 compiler, and network access on first build (GoogleTest is fetched automatically).

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Binaries:

- `build/wz-match` — CLI
- `build/warzone_tests` — unit tests

### Library usage

```cpp
#include "warzone/matcher.hpp"
#include "warzone/queue.hpp"
#include "warzone/types.hpp"

using namespace warzone;

ConsoleMatchmakingQueue queue;

Player xboxPlayer = createConsoleOnlyPlayer("xbox_1", Platform::XboxSeries);
xboxPlayer.region = "na-east";

Player psPlayer = createConsoleOnlyPlayer("ps5_1", Platform::Ps5);
psPlayer.region = "na-east";

queue.registerPlayer(xboxPlayer);
queue.registerPlayer(psPlayer);

queue.enqueue("xbox_1");
queue.enqueue("ps5_1");

const std::optional<QueueMatch> match = queue.tick();
// match->lobby contains a PC-free Xbox + PlayStation lobby
```

### CLI

```bash
# Can Xbox and PlayStation match? (yes)
./build/wz-match check xbox_series console_only ps5 console_only

# Can Xbox and PC match under console_only? (no)
./build/wz-match check xbox_series console_only pc all

# Build a lobby from player specs (PC excluded)
./build/wz-match lobby xbox_1:xbox_series:na-east ps5_1:ps5:na-east pc_1:pc:na-east

# End-to-end queue demo
./build/wz-match demo
```

Player spec format for `filter` and `lobby` commands:

```
id:platform[:region][:skill]
```

## Verifying it works

Run the test suite — 10 unit tests cover platform rules, party validation, pool filtering, and lobby formation:

```bash
ctest --test-dir build --output-on-failure
```

Use the CLI commands above to sanity-check specific platform pairs and lobby output.

## What this is not

- **Not** an official Activision or Call of Duty product
- **Not** connected to live Warzone matchmaking
- **Not** a cheat, exploit, or game modification

This is open-source matchmaking **logic** for tools and services that need console-only pool rules. Integrating with real game traffic would require your own backend and compliance with the game's terms of service.

## Console deployment

This repository is implemented in **C++** so it can be linked into console services, platform tools, or matchmaking backends using Xbox GDK, PlayStation SDK, or other native stacks.

To use it on a console platform:

1. Add the `warzone_matchmaking` library to your CMake or platform build
2. Wire player/platform telemetry into `Player` and `ConsoleMatchmakingQueue`
3. Use the unit tests as acceptance criteria for your integration

## License

Public repository. See repository settings for license details.
