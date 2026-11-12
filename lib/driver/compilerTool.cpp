// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/configure.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "common/version.h"
#include "driver/compiler.h"
#include "driver/options.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Driver {

#ifdef WASMEDGE_USE_LLVM
int compileModule(const Configure &Conf, Span<const Byte> Data,
                  const std::filesystem::path &OutputPath) noexcept {
  using namespace std::literals;

  Loader::Loader Loader(Conf);
  std::unique_ptr<AST::Module> Module;
  if (auto Res = Loader.parseModule(Data)) {
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

  LLVM::Compiler Compiler(Conf);
  if (auto Res = Compiler.checkConfigure(); !Res) {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Compiler Configure failed. Error code: {}"sv, Err);
    return EXIT_FAILURE;
  }
  LLVM::CodeGen CodeGen(Conf);
  if (auto Res = Compiler.compile(*Module); !Res) {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Compilation failed. Error code: {}"sv, Err);
    return EXIT_FAILURE;
  } else if (auto Res2 = CodeGen.codegen(Data, std::move(*Res), OutputPath);
             !Res2) {
    const auto Err = static_cast<uint32_t>(Res2.error());
    spdlog::error("Code Generation failed. Error code: {}"sv, Err);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
#endif

int Compiler([[maybe_unused]] struct DriverCompilerOptions &Opt) noexcept {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  setLoggingLevel(Opt);

#ifdef WASMEDGE_USE_LLVM

  Configure Conf = createProposalConfigure(Opt);

  using OptimizationLevel = CompilerConfigure::OptimizationLevel;
  static const std::array<std::pair<std::string_view, OptimizationLevel>, 6>
      OptimizationLevels{{{"0"sv, OptimizationLevel::O0},
                          {"1"sv, OptimizationLevel::O1},
                          {"2"sv, OptimizationLevel::O2},
                          {"3"sv, OptimizationLevel::O3},
                          {"s"sv, OptimizationLevel::Os},
                          {"z"sv, OptimizationLevel::Oz}}};
  const auto Level =
      std::find_if(OptimizationLevels.begin(), OptimizationLevels.end(),
                   [&Opt](const auto &L) {
                     return L.first == Opt.PropOptimizationLevel.value();
                   });
  if (Level != OptimizationLevels.end()) {
    Conf.getCompilerConfigure().setOptimizationLevel(Level->second);
  } else {
    spdlog::warn("Unknown --optimize value: \"{}\"; using 2. Valid values: 0, "
                 "1, 2, 3, s, z."sv,
                 Opt.PropOptimizationLevel.value());
    Conf.getCompilerConfigure().setOptimizationLevel(OptimizationLevel::O2);
  }

  // Set interpreter run mode here so the loader reads function bodies fully
  // (the AOT compiler needs the parsed instructions, not AOT symbols).
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::Interpreter);

  const auto InputPath =
      getAbsolutePath(Opt.WasmName.value(), "input wasm file"sv);
  const auto OutputPath = getAbsolutePath(Opt.SoName.value(), "output file"sv);
  if (!InputPath || !OutputPath) {
    return EXIT_FAILURE;
  }

  std::vector<Byte> Data;
  if (auto Res = Loader::Loader(Conf).loadFile(*InputPath)) {
    Data = std::move(*Res);
  } else {
    const auto Err = static_cast<uint32_t>(Res.error());
    spdlog::error("Load failed. Error code: {}"sv, Err);
    return EXIT_FAILURE;
  }

  if (Opt.ConfDumpIR.value()) {
    Conf.getCompilerConfigure().setDumpIR(true);
  }
  if (Opt.ConfInterruptible.value()) {
    Conf.getCompilerConfigure().setInterruptible(true);
  }
  setStatisticsConfigure(Opt, Conf);
  if (Opt.ConfGenericBinary.value()) {
    Conf.getCompilerConfigure().setGenericBinary(true);
  }
  if (u8string(OutputPath->extension()) == WASMEDGE_LIB_EXTENSION) {
    Conf.getCompilerConfigure().setOutputFormat(
        CompilerConfigure::OutputFormat::Native);
  }

  return compileModule(Conf, Data, *OutputPath);
#else
  spdlog::error("Compilation is not supported!"sv);

  return EXIT_FAILURE;
#endif
}

} // namespace Driver
} // namespace WasmEdge
