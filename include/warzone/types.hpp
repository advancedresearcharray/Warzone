#pragma once

#include <optional>
#include <string>
#include <vector>

namespace warzone {

enum class Platform {
  Pc,
  XboxSeries,
  XboxOne,
  Ps5,
  Ps4,
};

enum class CrossPlayPreference {
  All,
  ConsoleOnly,
  SamePlatform,
};

struct Player {
  std::string id;
  Platform platform;
  CrossPlayPreference crossPlayPreference;
  std::optional<int> skillRating;
  std::optional<std::string> region;
  std::optional<std::string> partyId;
};

struct Party {
  std::string id;
  std::vector<std::string> memberIds;
};

struct MatchmakingConfig {
  int squadSize = 4;
  int lobbySize = 150;
  bool requireSameRegion = true;
  int skillBand = 200;
};

struct MatchCandidate {
  std::vector<std::string> playerIds;
  std::vector<Platform> platforms;
  std::optional<int> averageSkill;
  std::optional<std::string> region;
};

struct RejectedPlayer {
  std::string playerId;
  std::string reason;
};

struct MatchResult {
  std::vector<MatchCandidate> accepted;
  std::vector<RejectedPlayer> rejected;
};

struct CompatibilityResult {
  bool compatible = false;
  std::optional<std::string> reason;
};

enum class QueueStatus {
  Idle,
  Searching,
  Matched,
};

struct QueueTicket {
  std::string ticketId;
  std::vector<std::string> playerIds;
  long long enqueuedAt = 0;
  QueueStatus status = QueueStatus::Idle;
  std::optional<std::string> region;
};

struct QueueMatch {
  std::string ticketId;
  std::vector<Player> lobby;
  long long formedAt = 0;
};

MatchmakingConfig defaultMatchmakingConfig();

const char* platformToString(Platform platform);
const char* crossPlayPreferenceToString(CrossPlayPreference preference);

}  // namespace warzone
