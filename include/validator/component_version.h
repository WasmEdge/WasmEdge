// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace WasmEdge {
namespace Validator {
namespace ComponentVersion {

/// Semantic version structure for component model interface versions
struct SemVer {
  uint32_t Major;
  uint32_t Minor;
  uint32_t Patch;
  std::string PreRelease;
  std::string BuildMetadata;
};

/// Parse a semantic version string according to semver.org
/// Returns nullopt if the string is not a valid semver
std::optional<SemVer> parseSemVer(std::string_view version);

/// Canonicalize a semantic version per Component Model spec:
/// - If major > 0: keep only major (e.g., "1.2.3" -> "1" / ".2.3")
/// - Else if minor > 0: keep major.minor (e.g., "0.2.6-rc.1" -> "0.2" / ".6-rc.1")
/// - Else: keep major.minor.patch (e.g., "0.0.1-alpha" -> "0.0.1" / "-alpha")
/// Returns (canonical_version, semver_suffix)
std::pair<std::string, std::string> canonicalizeSemVer(const SemVer &ver);
std::pair<std::string, std::string> canonicalizeSemVer(std::string_view version);

/// Check if two canonical versions are compatible for linking
/// Both versions must already be canonical (i.e., not include patch/suffix unless 0.0.x)
bool areVersionsCompatible(std::string_view canonVersion1, 
                           std::string_view canonVersion2);

} // namespace ComponentVersion
} // namespace Validator
} // namespace WasmEdge
