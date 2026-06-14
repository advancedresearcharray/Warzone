#include "warzone/types.hpp"

namespace warzone {

MatchmakingConfig defaultMatchmakingConfig() { return MatchmakingConfig{}; }

const char* platformToString(Platform platform) {
  switch (platform) {
    case Platform::Pc:
      return "pc";
    case Platform::XboxSeries:
      return "xbox_series";
    case Platform::XboxOne:
      return "xbox_one";
    case Platform::Ps5:
      return "ps5";
    case Platform::Ps4:
      return "ps4";
  }
  return "unknown";
}

const char* crossPlayPreferenceToString(CrossPlayPreference preference) {
  switch (preference) {
    case CrossPlayPreference::All:
      return "all";
    case CrossPlayPreference::ConsoleOnly:
      return "console_only";
    case CrossPlayPreference::SamePlatform:
      return "same_platform";
  }
  return "unknown";
}

}  // namespace warzone
