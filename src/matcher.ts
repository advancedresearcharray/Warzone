import { isPc, platformsCompatible } from "./platform.js";
import {
  CrossPlayPreference,
  DEFAULT_MATCHMAKING_CONFIG,
  MatchCandidate,
  MatchmakingConfig,
  MatchResult,
  Party,
  Platform,
  Player,
} from "./types.js";

export interface CompatibilityResult {
  compatible: boolean;
  reason?: string;
}

/** Check if a single candidate player can join a lobby defined by existing members. */
export function canJoinLobby(
  existingPlayers: Player[],
  candidate: Player,
): CompatibilityResult {
  if (existingPlayers.length === 0) {
    return { compatible: true };
  }

  for (const member of existingPlayers) {
    const memberAllowsCandidate = platformsCompatible(
      member.platform,
      member.crossPlayPreference,
      candidate.platform,
    );
    const candidateAllowsMember = platformsCompatible(
      candidate.platform,
      candidate.crossPlayPreference,
      member.platform,
    );

    if (!memberAllowsCandidate || !candidateAllowsMember) {
      const blockedSide = !memberAllowsCandidate ? member.id : candidate.id;
      return {
        compatible: false,
        reason: `Platform mismatch between ${blockedSide} and ${candidate.id} (PC excluded for console-only)`,
      };
    }
  }

  if (existingPlayers[0]?.region && candidate.region) {
    if (existingPlayers[0].region !== candidate.region) {
      return {
        compatible: false,
        reason: `Region mismatch: ${candidate.region} vs ${existingPlayers[0].region}`,
      };
    }
  }

  return { compatible: true };
}

/** Filter a pool down to players eligible for console-only Warzone lobbies (no PC). */
export function filterConsoleOnlyPool(players: Player[]): MatchResult {
  const accepted: MatchCandidate[] = [];
  const rejected: MatchResult["rejected"] = [];

  for (const player of players) {
    if (isPc(player.platform)) {
      rejected.push({
        playerId: player.id,
        reason: "PC players excluded from console-only matchmaking",
      });
      continue;
    }

    if (player.crossPlayPreference === CrossPlayPreference.SAME_PLATFORM) {
      // Still valid in pool, but tagged — matching happens at lobby formation.
      accepted.push({
        playerIds: [player.id],
        platforms: [player.platform],
        averageSkill: player.skillRating,
        region: player.region,
      });
      continue;
    }

    accepted.push({
      playerIds: [player.id],
      platforms: [player.platform],
      averageSkill: player.skillRating,
      region: player.region,
    });
  }

  return { accepted, rejected };
}

/** Resolve a party's effective platform rules — strictest preference wins. */
export function resolvePartyPreference(members: Player[]): CrossPlayPreference {
  const order = [
    CrossPlayPreference.SAME_PLATFORM,
    CrossPlayPreference.CONSOLE_ONLY,
    CrossPlayPreference.ALL,
  ];

  for (const pref of order) {
    if (members.some((m) => m.crossPlayPreference === pref)) {
      return pref;
    }
  }

  return CrossPlayPreference.CONSOLE_ONLY;
}

/** Validate an entire party can queue together under console-only rules. */
export function validateParty(party: Party, playersById: Map<string, Player>): CompatibilityResult {
  const members = party.memberIds
    .map((id) => playersById.get(id))
    .filter((p): p is Player => p !== undefined);

  if (members.length !== party.memberIds.length) {
    return { compatible: false, reason: "Party contains unknown player IDs" };
  }

  if (members.some((m) => isPc(m.platform))) {
    return {
      compatible: false,
      reason: "Party includes a PC player — cannot enter console-only queue",
    };
  }

  for (let i = 0; i < members.length; i++) {
    for (let j = i + 1; j < members.length; j++) {
      const a = members[i];
      const b = members[j];
      const partyPref = resolvePartyPreference(members);

      const aAllowsB = platformsCompatible(a.platform, partyPref, b.platform);
      const bAllowsA = platformsCompatible(b.platform, partyPref, a.platform);

      if (!aAllowsB || !bAllowsA) {
        return {
          compatible: false,
          reason: `Party members ${a.id} (${a.platform}) and ${b.id} (${b.platform}) cannot play together under ${partyPref}`,
        };
      }
    }
  }

  return { compatible: true };
}

/** Build squads from a player pool using console-only compatibility rules. */
export function formSquads(
  players: Player[],
  config: MatchmakingConfig = DEFAULT_MATCHMAKING_CONFIG,
): { squads: Player[][]; unmatched: Player[] } {
  const pool = [...players];
  const squads: Player[][] = [];
  const unmatched: Player[] = [];

  while (pool.length > 0) {
    const seed = pool.shift();
    if (!seed) break;

    const squad: Player[] = [seed];

    for (let i = pool.length - 1; i >= 0 && squad.length < config.squadSize; i--) {
      const candidate = pool[i];
      const { compatible } = canJoinLobby(squad, candidate);

      if (!compatible) continue;

      if (config.skillBand > 0 && seed.skillRating !== undefined && candidate.skillRating !== undefined) {
        if (Math.abs(seed.skillRating - candidate.skillRating) > config.skillBand) {
          continue;
        }
      }

      squad.push(candidate);
      pool.splice(i, 1);
    }

    if (squad.length === config.squadSize) {
      squads.push(squad);
    } else {
      unmatched.push(...squad);
    }
  }

  return { squads, unmatched };
}

/** Form a full lobby from squads / solo players. */
export function formLobby(
  players: Player[],
  config: MatchmakingConfig = DEFAULT_MATCHMAKING_CONFIG,
): { lobby: Player[]; overflow: Player[] } {
  const { accepted, rejected } = filterConsoleOnlyPool(players);

  if (rejected.length > 0) {
    // PC already filtered — continue with accepted solo entries mapped back to Player objects.
  }

  const eligibleIds = new Set(accepted.flatMap((c) => c.playerIds));
  const eligible = players.filter((p) => eligibleIds.has(p.id));

  const lobby: Player[] = [];
  const overflow: Player[] = [];

  for (const player of eligible) {
    if (lobby.length >= config.lobbySize) {
      overflow.push(player);
      continue;
    }

    const { compatible } = canJoinLobby(lobby, player);
    if (compatible) {
      lobby.push(player);
    } else {
      overflow.push(player);
    }
  }

  return { lobby, overflow };
}

/** Summarize platform mix in a lobby — useful for telemetry / UI. */
export function lobbyPlatformSummary(players: Player[]): Record<Platform, number> {
  const summary: Record<Platform, number> = {
    [Platform.PC]: 0,
    [Platform.XBOX_SERIES]: 0,
    [Platform.XBOX_ONE]: 0,
    [Platform.PS5]: 0,
    [Platform.PS4]: 0,
  };

  for (const p of players) {
    summary[p.platform]++;
  }

  return summary;
}

/** True when lobby has zero PC players and at least one console platform. */
export function isConsoleOnlyLobby(players: Player[]): boolean {
  return players.length > 0 && players.every((p) => !isPc(p.platform));
}

/** Preset config for Warzone console players who want Xbox + PS but no PC. */
export function consoleOnlyWarzoneConfig(
  overrides: Partial<MatchmakingConfig> = {},
): MatchmakingConfig {
  return {
    ...DEFAULT_MATCHMAKING_CONFIG,
    ...overrides,
  };
}

export function createConsoleOnlyPlayer(
  id: string,
  platform: Exclude<Platform, Platform.PC>,
  options: Partial<Omit<Player, "id" | "platform">> = {},
): Player {
  return {
    id,
    platform,
    crossPlayPreference: CrossPlayPreference.CONSOLE_ONLY,
    ...options,
  };
}
