# Warzone

Console-focused matchmaking logic for Call of Duty: Warzone.

## The idea

Warzone supports cross-play between PC, Xbox, and PlayStation. That is great for population size, but many console players prefer **not** to match against PC — whether because of input differences, perceived advantage, or cheat concerns.

What console players often *do* want is **cross-play between consoles**: Xbox and PlayStation in the same lobby, without PC players mixed in.

This project is a **matchmaking rules module** that encodes that preference:

- **Block PC** players from console-only pools
- **Allow Xbox ↔ PlayStation** cross-play
- **Validate parties** so mixed console squads can queue together
- **Form lobbies and squads** using region and optional skill-band rules

It is a standalone TypeScript library and CLI — a building block you can plug into a lobby browser, party tool, private server, or backend service. It does **not** hook into Activision's live matchmaking servers.

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
src/
  types.ts      Player, party, and config types
  platform.ts   Platform detection and compatibility rules
  matcher.ts    Pool filtering, lobby/squad formation, party validation
  queue.ts      In-memory matchmaking queue
  cli.ts        Command-line tool for checks and demos
  index.ts      Public API exports
tests/
  matcher.test.ts
```

## Quick start

```bash
npm install
npm run build
npm test
```

### Library usage

```typescript
import {
  ConsoleMatchmakingQueue,
  Platform,
  createConsoleOnlyPlayer,
} from "./dist/index.js";

const queue = new ConsoleMatchmakingQueue();

queue.registerPlayer(
  createConsoleOnlyPlayer("xbox_1", Platform.XBOX_SERIES, { region: "na-east" }),
);
queue.registerPlayer(
  createConsoleOnlyPlayer("ps5_1", Platform.PS5, { region: "na-east" }),
);

queue.enqueue("xbox_1");
queue.enqueue("ps5_1");

const match = queue.tick();
// match.lobby → PC-free lobby with Xbox + PlayStation players
```

### CLI

```bash
# Can Xbox and PlayStation match? (yes)
node dist/cli.js check xbox_series console_only ps5 console_only

# Can Xbox and PC match under console_only? (no)
node dist/cli.js check xbox_series console_only pc all

# Build a lobby from player specs (PC excluded)
node dist/cli.js lobby xbox_1:xbox_series:na-east ps5_1:ps5:na-east pc_1:pc:na-east

# End-to-end queue demo
node dist/cli.js demo
```

Player spec format for `filter` and `lobby` commands:

```
id:platform[:region][:skill]
```

## Verifying it works

Run the test suite — 10 unit tests cover platform rules, party validation, pool filtering, and lobby formation:

```bash
npm test
```

Use the CLI commands above to sanity-check specific platform pairs and lobby output.

## What this is not

- **Not** an official Activision or Call of Duty product
- **Not** connected to live Warzone matchmaking
- **Not** a cheat, exploit, or game modification

This is open-source matchmaking **logic** for tools and services that need console-only pool rules. Integrating with real game traffic would require your own backend and compliance with the game's terms of service.

## Applying on console

This repository is a **reference implementation** in TypeScript. It does not run on Xbox or PlayStation as-is, and it cannot be injected into Warzone on console without a separate port.

To actually apply these rules in a console environment, the logic would need to be **recreated in console form** — reimplemented in the language, runtime, and APIs available on the target platform (for example C++ with the Xbox GDK or PlayStation SDK, or another supported console development stack).

Treat this repo as the specification:

- Platform compatibility rules (`console_only`, `same_platform`, `all`)
- Party and queue validation
- Lobby and squad formation behavior
- Unit tests as acceptance criteria for the port

The TypeScript here is a prototype and testbed. A console-native version would follow the same rules, but the code itself must be rewritten for the console platform you are targeting.

## License

Public repository. See repository settings for license details.
