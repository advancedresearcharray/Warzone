/** Supported Warzone platform identifiers. */
export enum Platform {
  PC = "pc",
  XBOX_SERIES = "xbox_series",
  XBOX_ONE = "xbox_one",
  PS5 = "ps5",
  PS4 = "ps4",
}

/** How a player wants cross-play to behave. */
export enum CrossPlayPreference {
  /** Match with any platform (default Warzone behavior). */
  ALL = "all",
  /** Console-only: Xbox + PlayStation, no PC. */
  CONSOLE_ONLY = "console_only",
  /** Same platform family only (e.g. PlayStation with PlayStation). */
  SAME_PLATFORM = "same_platform",
}

export interface Player {
  id: string;
  platform: Platform;
  crossPlayPreference: CrossPlayPreference;
  skillRating?: number;
  region?: string;
  partyId?: string;
}

export interface Party {
  id: string;
  memberIds: string[];
}

export interface MatchmakingConfig {
  /** Lobby size for a Warzone squad (default 4). */
  squadSize: number;
  /** Max players per match lobby (default 150 for BR). */
  lobbySize: number;
  /** Require same region when set. */
  requireSameRegion: boolean;
  /** Skill rating band for matching (0 = disabled). */
  skillBand: number;
}

export interface MatchCandidate {
  playerIds: string[];
  platforms: Platform[];
  averageSkill?: number;
  region?: string;
}

export interface MatchResult {
  accepted: MatchCandidate[];
  rejected: Array<{ playerId: string; reason: string }>;
}

export const DEFAULT_MATCHMAKING_CONFIG: MatchmakingConfig = {
  squadSize: 4,
  lobbySize: 150,
  requireSameRegion: true,
  skillBand: 200,
};
