#include "warzone/matcher.hpp"

#include "warzone/platform.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace warzone {

namespace {

MatchCandidate makeCandidate(const Player& player) {
  MatchCandidate candidate;
  candidate.playerIds = {player.id};
  candidate.platforms = {player.platform};
  candidate.averageSkill = player.skillRating;
  candidate.region = player.region;
  return candidate;
}

}  // namespace

CompatibilityResult canJoinLobby(const std::vector<Player>& existingPlayers,
                                 const Player& candidate) {
  if (existingPlayers.empty()) {
    return {true, std::nullopt};
  }

  for (const Player& member : existingPlayers) {
    const bool memberAllowsCandidate = platformsCompatible(
        member.platform, member.crossPlayPreference, candidate.platform);
    const bool candidateAllowsMember = platformsCompatible(
        candidate.platform, candidate.crossPlayPreference, member.platform);

    if (!memberAllowsCandidate || !candidateAllowsMember) {
      const std::string blockedSide =
          !memberAllowsCandidate ? member.id : candidate.id;
      std::ostringstream reason;
      reason << "Platform mismatch between " << blockedSide << " and "
             << candidate.id << " (PC excluded for console-only)";
      return {false, reason.str()};
    }
  }

  if (existingPlayers.front().region && candidate.region &&
      *existingPlayers.front().region != *candidate.region) {
    std::ostringstream reason;
    reason << "Region mismatch: " << *candidate.region << " vs "
           << *existingPlayers.front().region;
    return {false, reason.str()};
  }

  return {true, std::nullopt};
}

MatchResult filterConsoleOnlyPool(const std::vector<Player>& players) {
  MatchResult result;

  for (const Player& player : players) {
    if (isPc(player.platform)) {
      result.rejected.push_back(
          {player.id, "PC players excluded from console-only matchmaking"});
      continue;
    }

    result.accepted.push_back(makeCandidate(player));
  }

  return result;
}

CrossPlayPreference resolvePartyPreference(const std::vector<Player>& members) {
  const CrossPlayPreference order[] = {CrossPlayPreference::SamePlatform,
                                       CrossPlayPreference::ConsoleOnly,
                                       CrossPlayPreference::All};

  for (CrossPlayPreference pref : order) {
    if (std::any_of(members.begin(), members.end(),
                    [pref](const Player& member) {
                      return member.crossPlayPreference == pref;
                    })) {
      return pref;
    }
  }

  return CrossPlayPreference::ConsoleOnly;
}

CompatibilityResult validateParty(
    const Party& party,
    const std::unordered_map<std::string, Player>& playersById) {
  std::vector<Player> members;
  members.reserve(party.memberIds.size());

  for (const std::string& memberId : party.memberIds) {
    const auto it = playersById.find(memberId);
    if (it == playersById.end()) {
      return {false, "Party contains unknown player IDs"};
    }
    members.push_back(it->second);
  }

  if (std::any_of(members.begin(), members.end(),
                  [](const Player& member) { return isPc(member.platform); })) {
    return {false, "Party includes a PC player — cannot enter console-only queue"};
  }

  const CrossPlayPreference partyPref = resolvePartyPreference(members);

  for (size_t i = 0; i < members.size(); ++i) {
    for (size_t j = i + 1; j < members.size(); ++j) {
      const Player& a = members[i];
      const Player& b = members[j];

      const bool aAllowsB =
          platformsCompatible(a.platform, partyPref, b.platform);
      const bool bAllowsA =
          platformsCompatible(b.platform, partyPref, a.platform);

      if (!aAllowsB || !bAllowsA) {
        std::ostringstream reason;
        reason << "Party members " << a.id << " (" << platformToString(a.platform)
               << ") and " << b.id << " (" << platformToString(b.platform)
               << ") cannot play together under "
               << crossPlayPreferenceToString(partyPref);
        return {false, reason.str()};
      }
    }
  }

  return {true, std::nullopt};
}

SquadFormationResult formSquads(const std::vector<Player>& players,
                                const MatchmakingConfig& config) {
  SquadFormationResult result;
  std::vector<Player> pool = players;

  while (!pool.empty()) {
    Player seed = pool.front();
    pool.erase(pool.begin());

    std::vector<Player> squad = {seed};

    for (int i = static_cast<int>(pool.size()) - 1;
         i >= 0 && static_cast<int>(squad.size()) < config.squadSize; --i) {
      const Player& candidate = pool[static_cast<size_t>(i)];
      const CompatibilityResult compatibility = canJoinLobby(squad, candidate);
      if (!compatibility.compatible) {
        continue;
      }

      if (config.skillBand > 0 && seed.skillRating && candidate.skillRating) {
        if (std::abs(*seed.skillRating - *candidate.skillRating) > config.skillBand) {
          continue;
        }
      }

      squad.push_back(candidate);
      pool.erase(pool.begin() + i);
    }

    if (static_cast<int>(squad.size()) == config.squadSize) {
      result.squads.push_back(std::move(squad));
    } else {
      result.unmatched.insert(result.unmatched.end(), squad.begin(), squad.end());
    }
  }

  return result;
}

LobbyFormationResult formLobby(const std::vector<Player>& players,
                               const MatchmakingConfig& config) {
  LobbyFormationResult result;
  const MatchResult filtered = filterConsoleOnlyPool(players);

  std::set<std::string> eligibleIds;
  for (const MatchCandidate& candidate : filtered.accepted) {
    for (const std::string& playerId : candidate.playerIds) {
      eligibleIds.insert(playerId);
    }
  }

  for (const Player& player : players) {
    if (eligibleIds.find(player.id) == eligibleIds.end()) {
      continue;
    }

    if (static_cast<int>(result.lobby.size()) >= config.lobbySize) {
      result.overflow.push_back(player);
      continue;
    }

    const CompatibilityResult compatibility = canJoinLobby(result.lobby, player);
    if (compatibility.compatible) {
      result.lobby.push_back(player);
    } else {
      result.overflow.push_back(player);
    }
  }

  return result;
}

std::unordered_map<Platform, int> lobbyPlatformSummary(
    const std::vector<Player>& players) {
  std::unordered_map<Platform, int> summary = {
      {Platform::Pc, 0},
      {Platform::XboxSeries, 0},
      {Platform::XboxOne, 0},
      {Platform::Ps5, 0},
      {Platform::Ps4, 0},
  };

  for (const Player& player : players) {
    ++summary[player.platform];
  }

  return summary;
}

bool isConsoleOnlyLobby(const std::vector<Player>& players) {
  return !players.empty() &&
         std::all_of(players.begin(), players.end(),
                     [](const Player& player) { return !isPc(player.platform); });
}

MatchmakingConfig consoleOnlyWarzoneConfig(const MatchmakingConfig& overrides) {
  MatchmakingConfig config = defaultMatchmakingConfig();
  config.squadSize = overrides.squadSize;
  config.lobbySize = overrides.lobbySize;
  config.requireSameRegion = overrides.requireSameRegion;
  config.skillBand = overrides.skillBand;
  return config;
}

Player createConsoleOnlyPlayer(const std::string& id,
                               Platform platform,
                               const Player& defaults) {
  Player player = defaults;
  player.id = id;
  player.platform = platform;
  player.crossPlayPreference = CrossPlayPreference::ConsoleOnly;
  return player;
}

}  // namespace warzone
