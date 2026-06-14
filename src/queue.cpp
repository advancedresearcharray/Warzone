#include "warzone/queue.hpp"

#include <chrono>
#include <set>
#include <utility>

namespace warzone {

ConsoleMatchmakingQueue::ConsoleMatchmakingQueue(MatchmakingConfig config)
    : config_(std::move(config)) {}

void ConsoleMatchmakingQueue::registerPlayer(const Player& player) {
  players_[player.id] = player;
}

std::variant<RegisterPartySuccess, RegisterPartyFailure>
ConsoleMatchmakingQueue::registerParty(const Party& party) {
  const CompatibilityResult validation = validateParty(party, players_);
  if (!validation.compatible) {
    return RegisterPartyFailure{validation.reason.value_or("Invalid party")};
  }

  parties_[party.id] = party;
  for (const std::string& memberId : party.memberIds) {
    auto it = players_.find(memberId);
    if (it != players_.end()) {
      it->second.partyId = party.id;
    }
  }

  return RegisterPartySuccess{};
}

std::variant<EnqueueSuccess, EnqueueFailure> ConsoleMatchmakingQueue::enqueue(
    const std::string& playerOrPartyId) {
  const auto partyIt = parties_.find(playerOrPartyId);
  std::vector<std::string> playerIds;

  if (partyIt != parties_.end()) {
    playerIds = partyIt->second.memberIds;
  } else {
    playerIds = {playerOrPartyId};
  }

  for (const std::string& playerId : playerIds) {
    if (players_.find(playerId) == players_.end()) {
      return EnqueueFailure{"Unknown player or party"};
    }
  }

  if (partyIt != parties_.end()) {
    const CompatibilityResult validation = validateParty(partyIt->second, players_);
    if (!validation.compatible) {
      return EnqueueFailure{validation.reason.value_or("Party cannot queue")};
    }
  } else {
    const Player& solo = players_.at(playerOrPartyId);
    const MatchResult filtered = filterConsoleOnlyPool({solo});
    if (!filtered.rejected.empty()) {
      return EnqueueFailure{filtered.rejected.front().reason};
    }
    if (filtered.accepted.empty()) {
      return EnqueueFailure{"Player not eligible for console-only queue"};
    }
  }

  QueueTicket ticket;
  ticket.ticketId = "ticket_" + std::to_string(++ticketCounter_);
  ticket.playerIds = playerIds;
  ticket.enqueuedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  ticket.status = QueueStatus::Searching;

  const Player& firstMember = players_.at(playerIds.front());
  ticket.region = firstMember.region;

  tickets_[ticket.ticketId] = ticket;
  return EnqueueSuccess{ticket.ticketId};
}

std::optional<QueueMatch> ConsoleMatchmakingQueue::tick() {
  std::vector<QueueTicket> searching;
  for (const auto& [ticketId, ticket] : tickets_) {
    if (ticket.status == QueueStatus::Searching) {
      searching.push_back(ticket);
    }
  }

  if (searching.empty()) {
    return std::nullopt;
  }

  std::vector<Player> queuedPlayers;
  for (const QueueTicket& ticket : searching) {
    for (const std::string& playerId : ticket.playerIds) {
      queuedPlayers.push_back(players_.at(playerId));
    }
  }

  const LobbyFormationResult formed = formLobby(queuedPlayers, config_);
  if (formed.lobby.size() < 2) {
    return std::nullopt;
  }

  std::set<std::string> matchedIds;
  for (const Player& player : formed.lobby) {
    matchedIds.insert(player.id);
  }

  std::vector<QueueTicket> matchedTickets;
  for (const QueueTicket& ticket : searching) {
    const bool allMatched = std::all_of(
        ticket.playerIds.begin(), ticket.playerIds.end(),
        [&matchedIds](const std::string& playerId) {
          return matchedIds.find(playerId) != matchedIds.end();
        });

    if (allMatched) {
      matchedTickets.push_back(ticket);
    }
  }

  if (matchedTickets.empty()) {
    return std::nullopt;
  }

  for (const QueueTicket& ticket : matchedTickets) {
    QueueTicket updated = ticket;
    updated.status = QueueStatus::Matched;
    tickets_[ticket.ticketId] = updated;
  }

  QueueMatch match;
  match.ticketId = matchedTickets.front().ticketId;
  match.lobby = formed.lobby;
  match.formedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  return match;
}

std::optional<Player> ConsoleMatchmakingQueue::getPlayer(
    const std::string& id) const {
  const auto it = players_.find(id);
  if (it == players_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<QueueTicket> ConsoleMatchmakingQueue::getTicket(
    const std::string& ticketId) const {
  const auto it = tickets_.find(ticketId);
  if (it == tickets_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<std::vector<Player>> ConsoleMatchmakingQueue::fillSquads() const {
  std::vector<Player> searching;
  for (const auto& [ticketId, ticket] : tickets_) {
    if (ticket.status != QueueStatus::Searching) {
      continue;
    }
    for (const std::string& playerId : ticket.playerIds) {
      searching.push_back(players_.at(playerId));
    }
  }

  return formSquads(searching, config_).squads;
}

int ConsoleMatchmakingQueue::queueDepth() const {
  int depth = 0;
  for (const auto& [ticketId, ticket] : tickets_) {
    if (ticket.status == QueueStatus::Searching) {
      ++depth;
    }
  }
  return depth;
}

}  // namespace warzone
