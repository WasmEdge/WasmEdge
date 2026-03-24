// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "host/wasi/wasimodule.h"
#include "vm/vm.h"

#include <cstdlib>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

int InstantiateTool(struct DriverToolOptions &Opt) noexcept {
  std::ios::sync_with_stdio(false);

  Configure Conf = CreateConfigure(Opt);

  if (Opt.MemLim.value().size() > 0) {
    Conf.getRuntimeConfigure().setMaxMemoryPage(
        static_cast<uint32_t>(Opt.MemLim.value().back()));
  }

  Conf.addHostRegistration(HostRegistration::Wasi);
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));

  VM::VM VM(Conf);
  Host::WasiModule *WasiMod = dynamic_cast<Host::WasiModule *>(
      VM.getImportModule(HostRegistration::Wasi));

  if (auto Result = VM.loadWasm(InputPath.u8string()); !Result) {
    return EXIT_FAILURE;
  }
  if (auto Result = VM.validate(); !Result) {
    return EXIT_FAILURE;
  }
  if (auto Result = VM.instantiate(); !Result) {
    return EXIT_FAILURE;
  }

  WasiMod->init(Opt.Dir.value(),
                InputPath.filename()
                    .replace_extension(std::filesystem::u8path("wasm"sv))
                    .u8string(),
                Opt.Args.value(), Opt.Env.value());

  if (Opt.Reactor.value()) {
    auto Functions = VM.getFunctionList();
    for (const auto &Func : Functions) {
      if (Func.first == "_initialize") {
        if (auto Result = VM.execute("_initialize"sv); !Result) {
          return EXIT_FAILURE;
        }
        break;
      }
    }
  }

  spdlog::info("Instantiation succeeded.");
  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge