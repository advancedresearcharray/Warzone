#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "warzone/matcher.hpp"
#include "warzone/platform.hpp"
#include "warzone/queue.hpp"
#include "warzone/types.hpp"

namespace {

using namespace warzone;

void printHelp() {
  std::cout
      << "warzone-console-matchmaking — filter PC, allow Xbox + PlayStation\n\n"
      << "Usage:\n"
      << "  wz-match check <platform-a> <pref-a> <platform-b> <pref-b>\n"
      << "  wz-match filter <player-spec>...\n"
      << "  wz-match lobby <player-spec>...\n"
      << "  wz-match demo\n\n"
      << "Player spec: id:platform[:region][:skill]\n";
}

std::vector<std::string> split(const std::string& value, char delimiter) {
  std::vector<std::string> parts;
  std::stringstream stream(value);
  std::string part;
  while (std::getline(stream, part, delimiter)) {
    parts.push_back(part);
  }
  return parts;
}

Player parsePlayerSpec(const std::string& spec) {
  const std::vector<std::string> parts = split(spec, ':');
  if (parts.size() < 2) {
    throw std::runtime_error("Invalid player spec: " + spec);
  }

  const std::optional<Platform> platform = parsePlatform(parts[1]);
  if (!platform) {
    throw std::runtime_error("Unknown platform in spec: " + spec);
  }

  Player player;
  player.id = parts[0];
  player.platform = *platform;
  player.crossPlayPreference = CrossPlayPreference::ConsoleOnly;

  if (parts.size() >= 3 && !parts[2].empty()) {
    player.region = parts[2];
  }
  if (parts.size() >= 4 && !parts[3].empty()) {
    player.skillRating = std::stoi(parts[3]);
  }

  return player;
}

int cmdCheck(const std::vector<std::string>& args) {
  if (args.size() < 4) {
    std::cerr << "check requires 4 arguments\n";
    return 2;
  }

  const auto platformA = parsePlatform(args[0]);
  const auto prefA = parseCrossPlayPreference(args[1]);
  const auto platformB = parsePlatform(args[2]);
  const auto prefB = parseCrossPlayPreference(args[3]);

  if (!platformA || !platformB || !prefA || !prefB) {
    std::cerr << "Invalid platform or preference\n";
    return 2;
  }

  const bool aToB = platformsCompatible(*platformA, *prefA, *platformB);
  const bool bToA = platformsCompatible(*platformB, *prefB, *platformA);
  const bool compatible = aToB && bToA;

  std::cout << "platformA: " << platformToString(*platformA) << '\n'
            << "platformB: " << platformToString(*platformB) << '\n'
            << "compatible: " << (compatible ? "true" : "false") << '\n';

  return compatible ? 0 : 1;
}

int cmdFilter(const std::vector<std::string>& args) {
  std::vector<Player> players;
  for (const std::string& spec : args) {
    players.push_back(parsePlayerSpec(spec));
  }

  const MatchResult result = filterConsoleOnlyPool(players);
  std::cout << "Accepted: " << result.accepted.size() << '\n';
  for (const MatchCandidate& entry : result.accepted) {
    std::cout << "  + " << entry.playerIds.front() << '\n';
  }

  std::cout << "Rejected: " << result.rejected.size() << '\n';
  for (const RejectedPlayer& entry : result.rejected) {
    std::cout << "  - " << entry.playerId << ": " << entry.reason << '\n';
  }

  return 0;
}

int cmdLobby(const std::vector<std::string>& args) {
  std::vector<Player> players;
  for (const std::string& spec : args) {
    players.push_back(parsePlayerSpec(spec));
  }

  const LobbyFormationResult formed = formLobby(players);
  std::cout << "Console-only lobby: " << (isConsoleOnlyLobby(formed.lobby) ? "yes" : "no")
            << " (" << formed.lobby.size() << " players)\n";

  for (const Player& player : formed.lobby) {
    std::cout << "  IN  " << player.id << " (" << platformToString(player.platform)
              << ")\n";
  }
  for (const Player& player : formed.overflow) {
    std::cout << "  OUT " << player.id << " (" << platformToString(player.platform)
              << ")\n";
  }

  return 0;
}

int cmdDemo() {
  MatchmakingConfig config;
  config.lobbySize = 6;
  config.squadSize = 2;

  ConsoleMatchmakingQueue queue(config);

  Player xbox1Defaults;
  xbox1Defaults.region = "na-east";
  xbox1Defaults.skillRating = 1100;
  queue.registerPlayer(createConsoleOnlyPlayer("xbox_1", Platform::XboxSeries, xbox1Defaults));

  Player ps5Defaults;
  ps5Defaults.region = "na-east";
  ps5Defaults.skillRating = 1120;
  queue.registerPlayer(createConsoleOnlyPlayer("ps5_1", Platform::Ps5, ps5Defaults));

  Player ps4Defaults;
  ps4Defaults.region = "na-east";
  ps4Defaults.skillRating = 1080;
  queue.registerPlayer(createConsoleOnlyPlayer("ps4_1", Platform::Ps4, ps4Defaults));

  Player pcPlayer;
  pcPlayer.id = "pc_1";
  pcPlayer.platform = Platform::Pc;
  pcPlayer.crossPlayPreference = CrossPlayPreference::All;
  pcPlayer.skillRating = 1400;
  pcPlayer.region = "na-east";
  queue.registerPlayer(pcPlayer);

  Player xbox2Defaults;
  xbox2Defaults.region = "na-east";
  xbox2Defaults.skillRating = 1050;
  queue.registerPlayer(createConsoleOnlyPlayer("xbox_2", Platform::XboxOne, xbox2Defaults));

  const auto pcEnqueue = queue.enqueue("pc_1");
  if (std::holds_alternative<EnqueueFailure>(pcEnqueue)) {
    std::cout << "PC enqueue blocked: " << std::get<EnqueueFailure>(pcEnqueue).reason << '\n';
  }

  for (const std::string& id : {"xbox_1", "ps5_1", "ps4_1", "xbox_2"}) {
    const auto result = queue.enqueue(id);
    if (std::holds_alternative<EnqueueSuccess>(result)) {
      std::cout << "Enqueued " << id << ": " << std::get<EnqueueSuccess>(result).ticketId << '\n';
    }
  }

  const std::optional<QueueMatch> match = queue.tick();
  if (!match) {
    std::cout << "No match formed\n";
    return 0;
  }

  std::cout << "Match formed with " << match->lobby.size() << " players\n";
  for (const Player& player : match->lobby) {
    std::cout << "  " << player.id << " (" << platformToString(player.platform) << ")\n";
  }

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    printHelp();
    return 0;
  }

  const std::string command = argv[1];
  std::vector<std::string> args;
  for (int i = 2; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }

  try {
    if (command == "check") {
      return cmdCheck(args);
    }
    if (command == "filter") {
      return cmdFilter(args);
    }
    if (command == "lobby") {
      return cmdLobby(args);
    }
    if (command == "demo") {
      return cmdDemo();
    }

    std::cerr << "Unknown command: " << command << '\n';
    printHelp();
    return 2;
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
