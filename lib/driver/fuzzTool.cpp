// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#ifdef WASMEDGE_BUILD_FUZZING
#include "driver/fuzzTool.h"
#include "common/configure.h"
#include "common/spdlog.h"
#include "driver/compiler.h"

#include <cstdlib>
#include <string_view>

namespace WasmEdge {
namespace Driver {

int FuzzTool(const uint8_t *Data, size_t Size) noexcept {
  using namespace std::literals;
  std::ios::sync_with_stdio(false);
  spdlog::set_level(spdlog::level::critical);

#ifdef WASMEDGE_USE_LLVM
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::Interpreter);
  return compileModule(Conf, Span<const uint8_t>(Data, Size), "/dev/null"sv);
#else
  static_cast<void>(Data);
  static_cast<void>(Size);
  return EXIT_SUCCESS;
#endif
}

} // namespace Driver
} // namespace WasmEdge
#endif
