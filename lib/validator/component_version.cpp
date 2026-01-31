// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/component_version.h"

#include <charconv>
#include <sstream>

namespace WasmEdge {
namespace Validator {
namespace ComponentVersion {

namespace {
// Helper to parse a number from string_view
std::optional<uint32_t> parseNumber(std::string_view str) {
  uint32_t result;
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
  if (ec != std::errc() || ptr != str.data() + str.size()) {
    return std::nullopt;
  }
  return result;
}
} // namespace

std::optional<SemVer> parseSemVer(std::string_view version) {
  SemVer result{0, 0, 0, "", ""};
  
  // Find major.minor.patch
  size_t firstDot = version.find('.');
  if (firstDot == std::string_view::npos) {
    return std::nullopt;
  }
  
  auto majorOpt = parseNumber(version.substr(0, firstDot));
  if (!majorOpt) {
    return std::nullopt;
  }
  result.Major = *majorOpt;
  
  version.remove_prefix(firstDot + 1);
  
  // Find minor and patch
  size_t secondDot = version.find('.');
  if (secondDot == std::string_view::npos) {
    return std::nullopt;
  }
  
  auto minorOpt = parseNumber(version.substr(0, secondDot));
  if (!minorOpt) {
    return std::nullopt;
  }
  result.Minor = *minorOpt;
  
  version.remove_prefix(secondDot + 1);
  
  // Parse patch (may have -prerelease or +buildmetadata)
  size_t dashPos = version.find('-');
  size_t plusPos = version.find('+');
  size_t endOfPatch = std::min(dashPos, plusPos);
  
  std::string_view patchStr = version.substr(0, endOfPatch);
  auto patchOpt = parseNumber(patchStr);
  if (!patchOpt) {
    return std::nullopt;
  }
  result.Patch = *patchOpt;
  
  // Parse pre-release if present
  if (dashPos != std::string_view::npos) {
    size_t preReleaseEnd = (plusPos != std::string_view::npos && plusPos > dashPos) 
                           ? plusPos - dashPos - 1 
                           : version.size() - dashPos - 1;
    result.PreRelease = std::string(version.substr(dashPos + 1, preReleaseEnd));
  }
  
  // Parse build metadata if present
  if (plusPos != std::string_view::npos) {
    result.BuildMetadata = std::string(version.substr(plusPos + 1));
  }
  
  return result;
}

std::pair<std::string, std::string> canonicalizeSemVer(const SemVer &ver) {
  std::ostringstream canonical;
  std::ostringstream suffix;
  
  if (ver.Major > 0) {
    // major > 0: canonversion = "major", suffix = ".minor.patch..."
    canonical << ver.Major;
    suffix << "." << ver.Minor << "." << ver.Patch;
    if (!ver.PreRelease.empty()) {
      suffix << "-" << ver.PreRelease;
    }
    if (!ver.BuildMetadata.empty()) {
      suffix << "+" << ver.BuildMetadata;
    }
  } else if (ver.Minor > 0) {
    // major == 0, minor > 0: canonversion = "0.minor", suffix = ".patch..."
    canonical << "0." << ver.Minor;
    suffix << "." << ver.Patch;
    if (!ver.PreRelease.empty()) {
      suffix << "-" << ver.PreRelease;
    }
    if (!ver.BuildMetadata.empty()) {
      suffix << "+" << ver.BuildMetadata;
    }
  } else {
    // major == 0, minor == 0: canonversion = "0.0.patch", suffix = prerelease+build
    canonical << "0.0." << ver.Patch;
    if (!ver.PreRelease.empty()) {
      suffix << "-" << ver.PreRelease;
    }
    if (!ver.BuildMetadata.empty()) {
      suffix << "+" << ver.BuildMetadata;
    }
  }
  
  return {canonical.str(), suffix.str()};
}

std::pair<std::string, std::string> canonicalizeSemVer(std::string_view version) {
  auto semverOpt = parseSemVer(version);
  if (!semverOpt) {
    // If parsing fails, return the original as canonical with empty suffix
    // This maintains compatibility with non-semver versions
    return {std::string(version), ""};
  }
  return canonicalizeSemVer(*semverOpt);
}

bool areVersionsCompatible(std::string_view canonVersion1, 
                           std::string_view canonVersion2) {
  // Canonical versions are compatible if they are exactly equal
  // This is because canonicalization already reduces to the compatibility boundary:
  // - major versions are incompatible (different major in canon)
  // - within same major, minor versions should match (same major.minor in canon for 0.x)
  return canonVersion1 == canonVersion2;
}

} // namespace ComponentVersion
} // namespace Validator
} // namespace WasmEdge
