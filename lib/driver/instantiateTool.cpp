// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "vm/vm.h"

#include <cstdlib>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

int InstantiateTool(struct DriverToolOptions &Opt) noexcept {
  std::ios::sync_with_stdio(false);

  Configure Conf = createConfigure(Opt);

  setMemoryPageLimit(Opt, Conf);

  Conf.addHostRegistration(HostRegistration::Wasi);

  const auto InputPath = getInputPath(Opt);
  if (!InputPath) {
    return EXIT_FAILURE;
  }

  VM::VM VM(Conf);

  if (!registerLinkedModules(Opt, VM)) {
    return EXIT_FAILURE;
  }

  if (auto Result = VM.loadWasm(u8string(*InputPath)); !Result) {
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
