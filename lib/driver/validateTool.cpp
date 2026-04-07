// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/configure.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "vm/vm.h"

#include <cstdlib>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

int ValidateTool(struct DriverToolOptions &Opt) noexcept {

  std::ios::sync_with_stdio(false);

  Configure Conf = createConfigure(Opt);

  Conf.addHostRegistration(HostRegistration::Wasi);
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));

  // Create VM and get WASI module instance.
  VM::VM VM(Conf);

  // Load, validate, WASM or Component.
  if (auto Result = VM.loadWasm(InputPath.u8string()); !Result) {
    return EXIT_FAILURE;
  }

  if (auto Result = VM.validate(); !Result) {
    return EXIT_FAILURE;
  }

  spdlog::info("Validation succeeded."sv);
  return EXIT_SUCCESS;
}
} // namespace Driver
} // namespace WasmEdge
