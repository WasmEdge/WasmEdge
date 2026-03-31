// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "vm/vm.h"

#include <cstdlib>
#include <string>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

int InstantiateTool(struct DriverToolOptions &Opt) noexcept {
  std::ios::sync_with_stdio(false);

  Configure Conf = createConfigure(Opt);

  if (Opt.MemLim.value().size() > 0) {
    Conf.getRuntimeConfigure().setMaxMemoryPage(
        static_cast<uint32_t>(Opt.MemLim.value().back()));
  }

  Conf.addHostRegistration(HostRegistration::Wasi);
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));

  VM::VM VM(Conf);

  for (const auto &ModEntry : Opt.LinkedModules.value()) {
    auto Pos = ModEntry.find(':');
    if (Pos == std::string::npos) {
      spdlog::error("Invalid --module format: \"{}\". Expected name:path."sv,
                    ModEntry);
      return EXIT_FAILURE;
    }
    auto Name = ModEntry.substr(0, Pos);
    auto Path = std::filesystem::absolute(
        std::filesystem::u8path(ModEntry.substr(Pos + 1)));
    if (auto Result = VM.registerModule(Name, Path); !Result) {
      spdlog::error("Failed to register module \"{}\" from: {}"sv, Name,
                    Path.u8string());
      return EXIT_FAILURE;
    }
  }

  if (auto Result = VM.loadWasm(InputPath.u8string()); !Result) {
    return EXIT_FAILURE;
  }
  if (auto Result = VM.validate(); !Result) {
    return EXIT_FAILURE;
  }
  if (auto Result = VM.instantiate(); !Result) {
    return EXIT_FAILURE;
  }

  spdlog::info("Instantiation succeeded."sv);
  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
