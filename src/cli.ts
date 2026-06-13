#!/usr/bin/env node
import {
  ConsoleMatchmakingQueue,
  CrossPlayPreference,
  Platform,
  filterConsoleOnlyPool,
  formLobby,
  isConsoleOnlyLobby,
  lobbyPlatformSummary,
  parsePlatform,
  platformsCompatible,
} from "./index.js";
import type { Player } from "./types.js";

const HELP = `
warzone-console-matchmaking — filter PC, allow Xbox + PlayStation

Usage:
  wz-match check <platform-a> <pref-a> <platform-b> <pref-b>
      Check if two platforms can match (prefs: all | console_only | same_platform)

  wz-match filter [--json] <player-spec>...
      Filter a player list for console-only pools.
      player-spec format: id:platform[:region][:skill]

  wz-match lobby [--json] <player-spec>...
      Build a console-only lobby from the given players.

  wz-match demo
      Run an interactive console-only matchmaking demo.

Examples:
  wz-match check xbox_series console_only ps5 console_only
  wz-match check xbox_series console_only pc all
  wz-match filter ps5:p1:na-east:1200 xbox_one:p2:na-east:1150 pc:p3:na-east:1300
  wz-match lobby ps5:p1:na-east ps4:p2:na-east xbox_series:p3:na-east
`.trim();

function parsePreference(raw: string): CrossPlayPreference {
  const map: Record<string, CrossPlayPreference> = {
    all: CrossPlayPreference.ALL,
    console_only: CrossPlayPreference.CONSOLE_ONLY,
    console: CrossPlayPreference.CONSOLE_ONLY,
    same_platform: CrossPlayPreference.SAME_PLATFORM,
    same: CrossPlayPreference.SAME_PLATFORM,
  };
  const pref = map[raw.toLowerCase()];
  if (!pref) {
    throw new Error(`Unknown preference "${raw}". Use all, console_only, or same_platform.`);
  }
  return pref;
}

function parsePlayerSpec(spec: string): Player {
  const [id, platformRaw, region, skillRaw] = spec.split(":");
  if (!id || !platformRaw) {
    throw new Error(`Invalid player spec "${spec}". Expected id:platform[:region][:skill]`);
  }

  const platform = parsePlatform(platformRaw);
  if (!platform) {
    throw new Error(`Unknown platform "${platformRaw}" in spec "${spec}"`);
  }

  return {
    id,
    platform,
    crossPlayPreference: CrossPlayPreference.CONSOLE_ONLY,
    region,
    skillRating: skillRaw ? Number(skillRaw) : undefined,
  };
}

function cmdCheck(args: string[]): void {
  if (args.length < 4) {
    console.error("check requires 4 arguments: platform-a pref-a platform-b pref-b");
    process.exit(2);
  }

  const [platA, prefA, platB, prefB] = args;
  const platformA = parsePlatform(platA);
  const platformB = parsePlatform(platB);

  if (!platformA || !platformB) {
    console.error("Invalid platform name");
    process.exit(2);
  }

  const preferenceA = parsePreference(prefA);
  const preferenceB = parsePreference(prefB);

  const aToB = platformsCompatible(platformA, preferenceA, platformB);
  const bToA = platformsCompatible(platformB, preferenceB, platformA);
  const compatible = aToB && bToA;

  console.log(
    JSON.stringify(
      {
        platformA: platformA,
        platformB: platformB,
        preferenceA,
        preferenceB,
        compatible,
        reason: compatible
          ? "Platforms may match"
          : "Blocked — PC excluded or same-platform restriction",
      },
      null,
      2,
    ),
  );

  process.exit(compatible ? 0 : 1);
}

function cmdFilter(args: string[], json: boolean): void {
  const players = args.map(parsePlayerSpec);
  const result = filterConsoleOnlyPool(players);

  if (json) {
    console.log(JSON.stringify(result, null, 2));
    return;
  }

  console.log(`Accepted: ${result.accepted.length}`);
  for (const entry of result.accepted) {
    console.log(`  + ${entry.playerIds.join(", ")} [${entry.platforms.join(", ")}]`);
  }

  console.log(`Rejected: ${result.rejected.length}`);
  for (const entry of result.rejected) {
    console.log(`  - ${entry.playerId}: ${entry.reason}`);
  }
}

function cmdLobby(args: string[], json: boolean): void {
  const players = args.map(parsePlayerSpec);
  const { lobby, overflow } = formLobby(players);

  const payload = {
    consoleOnly: isConsoleOnlyLobby(lobby),
    lobbySize: lobby.length,
    platforms: lobbyPlatformSummary(lobby),
    lobby: lobby.map((p) => ({ id: p.id, platform: p.platform, region: p.region })),
    overflow: overflow.map((p) => ({ id: p.id, platform: p.platform, reason: "compatibility or capacity" })),
  };

  if (json) {
    console.log(JSON.stringify(payload, null, 2));
    return;
  }

  console.log(`Console-only lobby: ${payload.consoleOnly ? "yes" : "no"} (${payload.lobbySize} players)`);
  console.log("Platform mix:", payload.platforms);
  for (const p of payload.lobby) {
    console.log(`  IN  ${p.id} (${p.platform})`);
  }
  for (const p of payload.overflow) {
    console.log(`  OUT ${p.id} (${p.platform})`);
  }
}

function cmdDemo(): void {
  const queue = new ConsoleMatchmakingQueue({ lobbySize: 6, squadSize: 2 });

  const roster: Player[] = [
    { id: "xbox_1", platform: Platform.XBOX_SERIES, crossPlayPreference: CrossPlayPreference.CONSOLE_ONLY, region: "na-east", skillRating: 1100 },
    { id: "ps5_1", platform: Platform.PS5, crossPlayPreference: CrossPlayPreference.CONSOLE_ONLY, region: "na-east", skillRating: 1120 },
    { id: "ps4_1", platform: Platform.PS4, crossPlayPreference: CrossPlayPreference.CONSOLE_ONLY, region: "na-east", skillRating: 1080 },
    { id: "pc_1", platform: Platform.PC, crossPlayPreference: CrossPlayPreference.ALL, region: "na-east", skillRating: 1400 },
    { id: "xbox_2", platform: Platform.XBOX_ONE, crossPlayPreference: CrossPlayPreference.CONSOLE_ONLY, region: "na-east", skillRating: 1050 },
  ];

  for (const p of roster) queue.registerPlayer(p);

  const pcEnqueue = queue.enqueue("pc_1");
  console.log("PC enqueue:", pcEnqueue.ok ? "allowed into queue" : pcEnqueue.reason);

  for (const id of ["xbox_1", "ps5_1", "ps4_1", "xbox_2"]) {
    const result = queue.enqueue(id);
    console.log(`Enqueued ${id}:`, result.ok ? result.ticketId : result.reason);
  }

  const match = queue.tick();
  if (!match) {
    console.log("No match yet");
    return;
  }

  console.log("\nMatch formed:");
  console.log(JSON.stringify({
    players: match.lobby.map((p) => ({ id: p.id, platform: p.platform })),
    platforms: lobbyPlatformSummary(match.lobby),
    consoleOnly: isConsoleOnlyLobby(match.lobby),
  }, null, 2));
}

function main(): void {
  const argv = process.argv.slice(2);
  const command = argv[0];

  if (!command || command === "--help" || command === "-h") {
    console.log(HELP);
    return;
  }

  const json = argv.includes("--json");
  const positional = argv.filter((a) => a !== "--json").slice(1);

  try {
    switch (command) {
      case "check":
        cmdCheck(positional);
        break;
      case "filter":
        cmdFilter(positional, json);
        break;
      case "lobby":
        cmdLobby(positional, json);
        break;
      case "demo":
        cmdDemo();
        break;
      default:
        console.error(`Unknown command: ${command}\n\n${HELP}`);
        process.exit(2);
    }
  } catch (err) {
    console.error(err instanceof Error ? err.message : String(err));
    process.exit(1);
  }
}

main();
