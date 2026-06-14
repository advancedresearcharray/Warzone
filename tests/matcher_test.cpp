#include <gtest/gtest.h>

#include <unordered_map>

#include "warzone/matcher.hpp"
#include "warzone/platform.hpp"
#include "warzone/types.hpp"

using namespace warzone;

TEST(PlatformsCompatible, AllowsConsoleCrossPlay) {
  EXPECT_TRUE(platformsCompatible(Platform::XboxSeries, CrossPlayPreference::ConsoleOnly,
                                  Platform::Ps5));
  EXPECT_TRUE(platformsCompatible(Platform::Ps4, CrossPlayPreference::ConsoleOnly,
                                  Platform::XboxOne));
}

TEST(PlatformsCompatible, BlocksPcUnderConsoleOnly) {
  EXPECT_FALSE(platformsCompatible(Platform::XboxSeries, CrossPlayPreference::ConsoleOnly,
                                   Platform::Pc));
  EXPECT_TRUE(platformsCompatible(Platform::Pc, CrossPlayPreference::All, Platform::Ps5));
}

TEST(PlatformsCompatible, EnforcesSamePlatform) {
  EXPECT_FALSE(platformsCompatible(Platform::Ps5, CrossPlayPreference::SamePlatform,
                                   Platform::XboxSeries));
  EXPECT_TRUE(platformsCompatible(Platform::Ps5, CrossPlayPreference::SamePlatform,
                                  Platform::Ps4));
}

TEST(FilterConsoleOnlyPool, RemovesPcPlayers) {
  const std::vector<Player> players = {
      createConsoleOnlyPlayer("x1", Platform::XboxSeries),
      createConsoleOnlyPlayer("p1", Platform::Ps5),
      Player{.id = "pc1", .platform = Platform::Pc, .crossPlayPreference = CrossPlayPreference::All},
  };

  const MatchResult result = filterConsoleOnlyPool(players);
  EXPECT_EQ(result.accepted.size(), 2u);
  EXPECT_EQ(result.rejected.size(), 1u);
  EXPECT_EQ(result.rejected.front().playerId, "pc1");
}

TEST(CanJoinLobby, AllowsConsoleCrossPlay) {
  const Player xbox = createConsoleOnlyPlayer("x1", Platform::XboxSeries, Player{.region = "na-east"});
  const Player ps5 = createConsoleOnlyPlayer("p1", Platform::Ps5, Player{.region = "na-east"});
  EXPECT_TRUE(canJoinLobby({xbox}, ps5).compatible);
}

TEST(CanJoinLobby, RejectsPcInConsoleLobby) {
  const Player xbox = createConsoleOnlyPlayer("x1", Platform::XboxSeries, Player{.region = "na-east"});
  const Player pc = Player{
      .id = "pc1",
      .platform = Platform::Pc,
      .crossPlayPreference = CrossPlayPreference::All,
      .region = std::string("na-east"),
  };
  EXPECT_FALSE(canJoinLobby({xbox}, pc).compatible);
}

TEST(ValidateParty, RejectsPcMembers) {
  std::unordered_map<std::string, Player> players = {
      {"x1", createConsoleOnlyPlayer("x1", Platform::XboxSeries)},
      {"pc1", Player{.id = "pc1", .platform = Platform::Pc, .crossPlayPreference = CrossPlayPreference::All}},
  };

  const Party party{.id = "party1", .memberIds = {"x1", "pc1"}};
  EXPECT_FALSE(validateParty(party, players).compatible);
}

TEST(ValidateParty, AllowsMixedConsoleParty) {
  std::unordered_map<std::string, Player> players = {
      {"x1", createConsoleOnlyPlayer("x1", Platform::XboxSeries)},
      {"p1", createConsoleOnlyPlayer("p1", Platform::Ps5)},
  };

  const Party party{.id = "party1", .memberIds = {"x1", "p1"}};
  EXPECT_TRUE(validateParty(party, players).compatible);
}

TEST(FormLobby, BuildsPcFreeLobby) {
  const std::vector<Player> players = {
      createConsoleOnlyPlayer("x1", Platform::XboxSeries, Player{.region = "eu-west"}),
      createConsoleOnlyPlayer("p1", Platform::Ps5, Player{.region = "eu-west"}),
      createConsoleOnlyPlayer("p2", Platform::Ps4, Player{.region = "eu-west"}),
      Player{.id = "pc1", .platform = Platform::Pc, .crossPlayPreference = CrossPlayPreference::All, .region = "eu-west"},
  };

  const LobbyFormationResult formed = formLobby(players);
  EXPECT_TRUE(isConsoleOnlyLobby(formed.lobby));
  EXPECT_EQ(formed.lobby.size(), 3u);
}

TEST(FormSquads, GroupsCompatiblePlayers) {
  const std::vector<Player> players = {
      createConsoleOnlyPlayer("x1", Platform::XboxSeries,
                              Player{.skillRating = 1000, .region = "na-east"}),
      createConsoleOnlyPlayer("p1", Platform::Ps5,
                              Player{.skillRating = 1050, .region = "na-east"}),
  };

  MatchmakingConfig config;
  config.squadSize = 2;
  config.lobbySize = 150;
  config.requireSameRegion = true;
  config.skillBand = 200;

  const SquadFormationResult formed = formSquads(players, config);
  EXPECT_EQ(formed.squads.size(), 1u);
  EXPECT_EQ(formed.squads.front().size(), 2u);
}
