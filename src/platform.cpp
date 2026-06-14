#include "warzone/platform.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace warzone {

namespace {

std::string normalizeToken(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  for (char& c : value) {
    if (c == ' ' || c == '-') {
      c = '_';
    }
  }

  while (!value.empty() && value.front() == '_') {
    value.erase(value.begin());
  }
  while (!value.empty() && value.back() == '_') {
    value.pop_back();
  }

  return value;
}

}  // namespace

bool isPc(Platform platform) { return platform == Platform::Pc; }

bool isConsole(Platform platform) {
  return platform == Platform::XboxSeries || platform == Platform::XboxOne ||
         platform == Platform::Ps5 || platform == Platform::Ps4;
}

bool isXbox(Platform platform) {
  return platform == Platform::XboxSeries || platform == Platform::XboxOne;
}

bool isPlayStation(Platform platform) {
  return platform == Platform::Ps5 || platform == Platform::Ps4;
}

PlatformFamily platformFamily(Platform platform) {
  if (isPc(platform)) {
    return PlatformFamily::Pc;
  }
  if (isXbox(platform)) {
    return PlatformFamily::Xbox;
  }
  return PlatformFamily::PlayStation;
}

bool platformsCompatible(Platform requesterPlatform,
                         CrossPlayPreference requesterPreference,
                         Platform candidatePlatform) {
  if (requesterPlatform == candidatePlatform) {
    return true;
  }

  switch (requesterPreference) {
    case CrossPlayPreference::All:
      return true;
    case CrossPlayPreference::ConsoleOnly:
      if (isPc(candidatePlatform) || isPc(requesterPlatform)) {
        return false;
      }
      return isConsole(candidatePlatform);
    case CrossPlayPreference::SamePlatform:
      return platformFamily(requesterPlatform) == platformFamily(candidatePlatform);
  }

  return false;
}

std::optional<Platform> parsePlatform(const std::string& raw) {
  static const std::unordered_map<std::string, Platform> map = {
      {"pc", Platform::Pc},
      {"win", Platform::Pc},
      {"windows", Platform::Pc},
      {"battlenet", Platform::Pc},
      {"steam", Platform::Pc},
      {"xbox_series", Platform::XboxSeries},
      {"xbox_series_x", Platform::XboxSeries},
      {"xbox_series_s", Platform::XboxSeries},
      {"xbox_scarlett", Platform::XboxSeries},
      {"xbox_one", Platform::XboxOne},
      {"xbox", Platform::XboxOne},
      {"ps5", Platform::Ps5},
      {"playstation_5", Platform::Ps5},
      {"ps4", Platform::Ps4},
      {"playstation_4", Platform::Ps4},
      {"playstation", Platform::Ps4},
  };

  const auto it = map.find(normalizeToken(raw));
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<CrossPlayPreference> parseCrossPlayPreference(const std::string& raw) {
  static const std::unordered_map<std::string, CrossPlayPreference> map = {
      {"all", CrossPlayPreference::All},
      {"console_only", CrossPlayPreference::ConsoleOnly},
      {"console", CrossPlayPreference::ConsoleOnly},
      {"same_platform", CrossPlayPreference::SamePlatform},
      {"same", CrossPlayPreference::SamePlatform},
  };

  const auto it = map.find(normalizeToken(raw));
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace warzone
