import { CrossPlayPreference, Platform } from "./types.js";

const CONSOLE_PLATFORMS: ReadonlySet<Platform> = new Set([
  Platform.XBOX_SERIES,
  Platform.XBOX_ONE,
  Platform.PS5,
  Platform.PS4,
]);

const XBOX_PLATFORMS: ReadonlySet<Platform> = new Set([
  Platform.XBOX_SERIES,
  Platform.XBOX_ONE,
]);

const PLAYSTATION_PLATFORMS: ReadonlySet<Platform> = new Set([
  Platform.PS5,
  Platform.PS4,
]);

export function isPc(platform: Platform): boolean {
  return platform === Platform.PC;
}

export function isConsole(platform: Platform): boolean {
  return CONSOLE_PLATFORMS.has(platform);
}

export function isXbox(platform: Platform): boolean {
  return XBOX_PLATFORMS.has(platform);
}

export function isPlayStation(platform: Platform): boolean {
  return PLAYSTATION_PLATFORMS.has(platform);
}

export function platformFamily(platform: Platform): "pc" | "xbox" | "playstation" {
  if (isPc(platform)) return "pc";
  if (isXbox(platform)) return "xbox";
  return "playstation";
}

/**
 * Returns true when two platforms may appear in the same lobby under the
 * requesting player's cross-play preference.
 */
export function platformsCompatible(
  requesterPlatform: Platform,
  requesterPreference: CrossPlayPreference,
  candidatePlatform: Platform,
): boolean {
  if (requesterPlatform === candidatePlatform) {
    return true;
  }

  switch (requesterPreference) {
    case CrossPlayPreference.ALL:
      return true;

    case CrossPlayPreference.CONSOLE_ONLY:
      // Block PC entirely; allow Xbox <-> PlayStation.
      if (isPc(candidatePlatform)) return false;
      if (isPc(requesterPlatform)) return false;
      return isConsole(candidatePlatform);

    case CrossPlayPreference.SAME_PLATFORM:
      return platformFamily(requesterPlatform) === platformFamily(candidatePlatform);

    default:
      return false;
  }
}

/** Normalize legacy or shorthand platform strings from APIs / telemetry. */
export function parsePlatform(raw: string): Platform | null {
  const normalized = raw.trim().toLowerCase().replace(/[\s-]+/g, "_");

  const map: Record<string, Platform> = {
    pc: Platform.PC,
    win: Platform.PC,
    windows: Platform.PC,
    battlenet: Platform.PC,
    steam: Platform.PC,
    xbox_series: Platform.XBOX_SERIES,
    xbox_series_x: Platform.XBOX_SERIES,
    xbox_series_s: Platform.XBOX_SERIES,
    xbox_scarlett: Platform.XBOX_SERIES,
    xbox_one: Platform.XBOX_ONE,
    xbox: Platform.XBOX_ONE,
    ps5: Platform.PS5,
    playstation_5: Platform.PS5,
    ps4: Platform.PS4,
    playstation_4: Platform.PS4,
    playstation: Platform.PS4,
  };

  return map[normalized] ?? null;
}
