import {
  CrossPlayPreference,
  Platform,
  canJoinLobby,
  createConsoleOnlyPlayer,
  filterConsoleOnlyPool,
  formLobby,
  formSquads,
  isConsoleOnlyLobby,
  platformsCompatible,
  validateParty,
} from "../src/index.js";
import type { Party, Player } from "../src/types.js";

describe("platformsCompatible", () => {
  it("allows Xbox and PlayStation under console_only", () => {
    expect(
      platformsCompatible(
        Platform.XBOX_SERIES,
        CrossPlayPreference.CONSOLE_ONLY,
        Platform.PS5,
      ),
    ).toBe(true);
    expect(
      platformsCompatible(
        Platform.PS4,
        CrossPlayPreference.CONSOLE_ONLY,
        Platform.XBOX_ONE,
      ),
    ).toBe(true);
  });

  it("blocks PC under console_only", () => {
    expect(
      platformsCompatible(
        Platform.XBOX_SERIES,
        CrossPlayPreference.CONSOLE_ONLY,
        Platform.PC,
      ),
    ).toBe(false);
    expect(
      platformsCompatible(
        Platform.PC,
        CrossPlayPreference.ALL,
        Platform.PS5,
      ),
    ).toBe(true);
  });

  it("enforces same_platform when selected", () => {
    expect(
      platformsCompatible(
        Platform.PS5,
        CrossPlayPreference.SAME_PLATFORM,
        Platform.XBOX_SERIES,
      ),
    ).toBe(false);
    expect(
      platformsCompatible(
        Platform.PS5,
        CrossPlayPreference.SAME_PLATFORM,
        Platform.PS4,
      ),
    ).toBe(true);
  });
});

describe("filterConsoleOnlyPool", () => {
  it("removes PC players and keeps consoles", () => {
    const players: Player[] = [
      createConsoleOnlyPlayer("x1", Platform.XBOX_SERIES),
      createConsoleOnlyPlayer("p1", Platform.PS5),
      { id: "pc1", platform: Platform.PC, crossPlayPreference: CrossPlayPreference.ALL },
    ];

    const result = filterConsoleOnlyPool(players);
    expect(result.accepted).toHaveLength(2);
    expect(result.rejected).toHaveLength(1);
    expect(result.rejected[0].playerId).toBe("pc1");
  });
});

describe("canJoinLobby", () => {
  const xbox = createConsoleOnlyPlayer("x1", Platform.XBOX_SERIES, { region: "na-east" });
  const ps5 = createConsoleOnlyPlayer("p1", Platform.PS5, { region: "na-east" });
  const pc = { id: "pc1", platform: Platform.PC, crossPlayPreference: CrossPlayPreference.ALL, region: "na-east" };

  it("allows console cross-play in the same lobby", () => {
    expect(canJoinLobby([xbox], ps5).compatible).toBe(true);
  });

  it("rejects PC in a console lobby", () => {
    expect(canJoinLobby([xbox], pc).compatible).toBe(false);
  });
});

describe("validateParty", () => {
  it("rejects parties with PC members for console queue", () => {
    const players = new Map<string, Player>([
      ["x1", createConsoleOnlyPlayer("x1", Platform.XBOX_SERIES)],
      ["pc1", { id: "pc1", platform: Platform.PC, crossPlayPreference: CrossPlayPreference.ALL }],
    ]);

    const party: Party = { id: "party1", memberIds: ["x1", "pc1"] };
    expect(validateParty(party, players).compatible).toBe(false);
  });

  it("allows mixed Xbox + PlayStation party", () => {
    const players = new Map<string, Player>([
      ["x1", createConsoleOnlyPlayer("x1", Platform.XBOX_SERIES)],
      ["p1", createConsoleOnlyPlayer("p1", Platform.PS5)],
    ]);

    const party: Party = { id: "party1", memberIds: ["x1", "p1"] };
    expect(validateParty(party, players).compatible).toBe(true);
  });
});

describe("formLobby", () => {
  it("builds a PC-free lobby with Xbox and PlayStation", () => {
    const players: Player[] = [
      createConsoleOnlyPlayer("x1", Platform.XBOX_SERIES, { region: "eu-west" }),
      createConsoleOnlyPlayer("p1", Platform.PS5, { region: "eu-west" }),
      createConsoleOnlyPlayer("p2", Platform.PS4, { region: "eu-west" }),
      { id: "pc1", platform: Platform.PC, crossPlayPreference: CrossPlayPreference.ALL, region: "eu-west" },
    ];

    const { lobby } = formLobby(players);
    expect(isConsoleOnlyLobby(lobby)).toBe(true);
    expect(lobby).toHaveLength(3);
    expect(lobby.some((p) => p.platform === Platform.PC)).toBe(false);
  });
});

describe("formSquads", () => {
  it("groups compatible console players into squads", () => {
    const players: Player[] = [
      createConsoleOnlyPlayer("x1", Platform.XBOX_SERIES, { skillRating: 1000, region: "na-east" }),
      createConsoleOnlyPlayer("p1", Platform.PS5, { skillRating: 1050, region: "na-east" }),
    ];

    const { squads } = formSquads(players, { squadSize: 2, lobbySize: 150, requireSameRegion: true, skillBand: 200 });
    expect(squads).toHaveLength(1);
    expect(squads[0]).toHaveLength(2);
  });
});
