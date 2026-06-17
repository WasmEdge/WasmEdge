// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "driver/unitool.h"
#include "common/spdlog.h"
#include "plugin/plugin.h"

#include <cstdlib>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

int PluginsTool() noexcept {
  std::ios::sync_with_stdio(false);

  Plugin::Plugin::loadFromDefaultPaths();
  const auto &Plugins = Plugin::Plugin::plugins();

  if (Plugins.empty()) {
    fmt::print("No plugins loaded.\n"sv);
    return EXIT_SUCCESS;
  }

  fmt::print("Loaded Plugins ({}):\n\n"sv, Plugins.size());
  for (const auto &P : Plugins) {
    const auto Ver = P.version();
    fmt::print("  \"{}\" v{}.{}.{}.{}\n"sv, P.name(), Ver.Major, Ver.Minor,
               Ver.Patch, Ver.Build);
    fmt::print("  path: {}\n"sv, P.path().string());

    const char *Desc = P.description();
    if (Desc && Desc[0] != '\0') {
      fmt::print("  description: {}\n"sv, Desc);
    }

    const auto Mods = P.modules();
    if (!Mods.empty()) {
      fmt::print("  modules ({}):\n"sv, Mods.size());
      for (const auto &M : Mods) {
        const char *ModDesc = M.description();
        if (ModDesc && ModDesc[0] != '\0') {
          fmt::print("    - \"{}\": {}\n"sv, M.name(), ModDesc);
        } else {
          fmt::print("    - \"{}\"\n"sv, M.name());
        }
      }
    }

    const auto Comps = P.components();
    if (!Comps.empty()) {
      fmt::print("  components ({}):\n"sv, Comps.size());
      for (const auto &C : Comps) {
        const char *CompDesc = C.description();
        if (CompDesc && CompDesc[0] != '\0') {
          fmt::print("    - \"{}\": {}\n"sv, C.name(), CompDesc);
        } else {
          fmt::print("    - \"{}\"\n"sv, C.name());
        }
      }
    }

    fmt::print("\n"sv);
  }

  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
