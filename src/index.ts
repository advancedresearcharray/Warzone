export {
  canJoinLobby,
  createConsoleOnlyPlayer,
  filterConsoleOnlyPool,
  formLobby,
  formSquads,
  isConsoleOnlyLobby,
  lobbyPlatformSummary,
  consoleOnlyWarzoneConfig,
  resolvePartyPreference,
  validateParty,
} from "./matcher.js";

export {
  isConsole,
  isPc,
  isPlayStation,
  isXbox,
  parsePlatform,
  platformFamily,
  platformsCompatible,
} from "./platform.js";

export { ConsoleMatchmakingQueue } from "./queue.js";

export {
  CrossPlayPreference,
  DEFAULT_MATCHMAKING_CONFIG,
  Platform,
} from "./types.js";

export type {
  CompatibilityResult,
} from "./matcher.js";

export type {
  MatchCandidate,
  MatchmakingConfig,
  MatchResult,
  Party,
  Player,
} from "./types.js";

export type { QueueMatch, QueueTicket } from "./queue.js";
