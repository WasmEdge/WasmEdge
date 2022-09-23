// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifdef WASMEDGE_BUILD_FUZZING
#include "driver/fuzzTool.h"
#include "aot/compiler.h"
#include "common/configure.h"
#include "loader/loader.h"
#include "validator/validator.h"

namespace WasmEdge {
namespace Driver {

int FuzzTool(const uint8_t *Data, size_t Size) noexcept {
  using namespace std::literals;
  std::ios::sync_with_stdio(false);
  spdlog::set_level(spdlog::level::critical);

  Configure Conf;
  Loader::Loader Loader(Conf);

  std::unique_ptr<AST::Module> Module;
  if (auto Res = Loader.parseModule(Span<const uint8_t>(Data, Size))) {
    Module = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Parse Module failed. Error code: {}"sv, Err);
    return EXIT_FAILURE;
  }

  {
    Validator::Validator ValidatorEngine(Conf);
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      const auto Err = static_cast<uint32_t>(Res.error());
      spdlog::error("Validate Module failed. Error code: {}"sv, Err);
      return EXIT_FAILURE;
    }
  }

  AOT::Compiler Compiler(Conf);
  if (auto Res = Compiler.compile(Span<const uint8_t>(Data, Size), *Module,
                                  "/dev/null"sv);
      !Res) {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Compilation failed. Error code: {}"sv, Err);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
#endif
