#pragma once

#include <unordered_map>
#include <vector>

#include "warzone/types.hpp"

namespace warzone {

CompatibilityResult canJoinLobby(const std::vector<Player>& existingPlayers,
                               const Player& candidate);

MatchResult filterConsoleOnlyPool(const std::vector<Player>& players);

CrossPlayPreference resolvePartyPreference(const std::vector<Player>& members);

CompatibilityResult validateParty(const Party& party,
                                  const std::unordered_map<std::string, Player>& playersById);

struct SquadFormationResult {
  std::vector<std::vector<Player>> squads;
  std::vector<Player> unmatched;
};

SquadFormationResult formSquads(const std::vector<Player>& players,
                                const MatchmakingConfig& config = defaultMatchmakingConfig());

struct LobbyFormationResult {
  std::vector<Player> lobby;
  std::vector<Player> overflow;
};

LobbyFormationResult formLobby(const std::vector<Player>& players,
                               const MatchmakingConfig& config = defaultMatchmakingConfig());

std::unordered_map<Platform, int> lobbyPlatformSummary(const std::vector<Player>& players);

bool isConsoleOnlyLobby(const std::vector<Player>& players);

MatchmakingConfig consoleOnlyWarzoneConfig(const MatchmakingConfig& overrides = {});

Player createConsoleOnlyPlayer(const std::string& id,
                               Platform platform,
                               const Player& defaults = {});

}  // namespace warzone
