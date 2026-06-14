#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "warzone/matcher.hpp"
#include "warzone/types.hpp"

namespace warzone {

struct EnqueueSuccess {
  std::string ticketId;
};

struct EnqueueFailure {
  std::string reason;
};

struct RegisterPartySuccess {};

struct RegisterPartyFailure {
  std::string reason;
};

class ConsoleMatchmakingQueue {
 public:
  explicit ConsoleMatchmakingQueue(MatchmakingConfig config = defaultMatchmakingConfig());

  void registerPlayer(const Player& player);

  std::variant<RegisterPartySuccess, RegisterPartyFailure> registerParty(const Party& party);

  std::variant<EnqueueSuccess, EnqueueFailure> enqueue(const std::string& playerOrPartyId);

  std::optional<QueueMatch> tick();

  std::optional<Player> getPlayer(const std::string& id) const;

  std::optional<QueueTicket> getTicket(const std::string& ticketId) const;

  std::vector<std::vector<Player>> fillSquads() const;

  int queueDepth() const;

 private:
  std::unordered_map<std::string, Player> players_;
  std::unordered_map<std::string, Party> parties_;
  std::unordered_map<std::string, QueueTicket> tickets_;
  MatchmakingConfig config_;
  int ticketCounter_ = 0;
};

}  // namespace warzone
