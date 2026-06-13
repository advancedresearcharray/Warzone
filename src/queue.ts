import { filterConsoleOnlyPool, formLobby, formSquads, validateParty } from "./matcher.js";
import {
  DEFAULT_MATCHMAKING_CONFIG,
  MatchmakingConfig,
  Party,
  Player,
} from "./types.js";

export type QueueStatus = "idle" | "searching" | "matched";

export interface QueueTicket {
  ticketId: string;
  playerIds: string[];
  enqueuedAt: number;
  status: QueueStatus;
  region?: string;
}

export interface QueueMatch {
  ticketId: string;
  lobby: Player[];
  formedAt: number;
}

/**
 * In-memory matchmaking queue for Warzone console-only sessions.
 * Production deployments would back this with Redis / dedicated servers.
 */
export class ConsoleMatchmakingQueue {
  private readonly players = new Map<string, Player>();
  private readonly parties = new Map<string, Party>();
  private readonly tickets = new Map<string, QueueTicket>();
  private readonly config: MatchmakingConfig;
  private ticketCounter = 0;

  constructor(config: Partial<MatchmakingConfig> = {}) {
    this.config = { ...DEFAULT_MATCHMAKING_CONFIG, ...config };
  }

  registerPlayer(player: Player): void {
    this.players.set(player.id, player);
  }

  registerParty(party: Party): { ok: true } | { ok: false; reason: string } {
    const validation = validateParty(party, this.players);
    if (!validation.compatible) {
      return { ok: false, reason: validation.reason ?? "Invalid party" };
    }

    this.parties.set(party.id, party);
    for (const memberId of party.memberIds) {
      const member = this.players.get(memberId);
      if (member) {
        this.players.set(memberId, { ...member, partyId: party.id });
      }
    }

    return { ok: true };
  }

  /** Enqueue one player or an entire party by ID. */
  enqueue(playerOrPartyId: string): { ok: true; ticketId: string } | { ok: false; reason: string } {
    const party = this.parties.get(playerOrPartyId);
    const playerIds = party ? party.memberIds : [playerOrPartyId];

    if (playerIds.some((id) => !this.players.has(id))) {
      return { ok: false, reason: "Unknown player or party" };
    }

    if (party) {
      const validation = validateParty(party, this.players);
      if (!validation.compatible) {
        return { ok: false, reason: validation.reason ?? "Party cannot queue" };
      }
    } else {
      const solo = this.players.get(playerOrPartyId)!;
      const { accepted, rejected } = filterConsoleOnlyPool([solo]);
      if (rejected.length > 0) {
        return { ok: false, reason: rejected[0].reason };
      }
      if (accepted.length === 0) {
        return { ok: false, reason: "Player not eligible for console-only queue" };
      }
    }

    const ticketId = `ticket_${++this.ticketCounter}`;
    const members = playerIds.map((id) => this.players.get(id)!);
    const region = members[0]?.region;

    this.tickets.set(ticketId, {
      ticketId,
      playerIds,
      enqueuedAt: Date.now(),
      status: "searching",
      region,
    });

    return { ok: true, ticketId };
  }

  /** Attempt to match all searching tickets into a lobby. */
  tick(): QueueMatch | null {
    const searching = [...this.tickets.values()].filter((t) => t.status === "searching");
    if (searching.length === 0) return null;

    const queuedPlayers = searching.flatMap((ticket) =>
      ticket.playerIds.map((id) => this.players.get(id)!),
    );

    const { lobby } = formLobby(queuedPlayers, this.config);
    if (lobby.length < 2) return null;

    const matchedIds = new Set(lobby.map((p) => p.id));
    const matchedTickets = searching.filter((t) =>
      t.playerIds.every((id) => matchedIds.has(id)),
    );

    if (matchedTickets.length === 0) return null;

    for (const ticket of matchedTickets) {
      this.tickets.set(ticket.ticketId, { ...ticket, status: "matched" });
    }

    return {
      ticketId: matchedTickets[0].ticketId,
      lobby,
      formedAt: Date.now(),
    };
  }

  getPlayer(id: string): Player | undefined {
    return this.players.get(id);
  }

  getTicket(ticketId: string): QueueTicket | undefined {
    return this.tickets.get(ticketId);
  }

  /** Fill-back squads for players still searching without a full lobby. */
  fillSquads(): Player[][] {
    const searching = [...this.tickets.values()]
      .filter((t) => t.status === "searching")
      .flatMap((t) => t.playerIds.map((id) => this.players.get(id)!));

    return formSquads(searching, this.config).squads;
  }

  queueDepth(): number {
    return [...this.tickets.values()].filter((t) => t.status === "searching").length;
  }
}
