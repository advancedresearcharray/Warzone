#pragma once

#include <optional>
#include <string>

#include "warzone/types.hpp"

namespace warzone {

bool isPc(Platform platform);
bool isConsole(Platform platform);
bool isXbox(Platform platform);
bool isPlayStation(Platform platform);

enum class PlatformFamily { Pc, Xbox, PlayStation };

PlatformFamily platformFamily(Platform platform);

bool platformsCompatible(Platform requesterPlatform,
                         CrossPlayPreference requesterPreference,
                         Platform candidatePlatform);

std::optional<Platform> parsePlatform(const std::string& raw);
std::optional<CrossPlayPreference> parseCrossPlayPreference(const std::string& raw);

}  // namespace warzone
