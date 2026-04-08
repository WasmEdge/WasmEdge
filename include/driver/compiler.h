// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/compiler.h - Compiler entrypoint ------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the entrypoint for the compiler executable.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "driver/options.h"
#include "po/argument_parser.h"
#include <string_view>

namespace WasmEdge {
namespace Driver {

using namespace std::literals;

struct DriverCompilerOptions : public DriverProposalOptions {
  DriverCompilerOptions()
      : WasmName(PO::Description("Wasm file"sv), PO::MetaVar("WASM"sv)),
        SoName(PO::Description("Wasm so file"sv), PO::MetaVar("WASM_SO"sv)),
        ConfGenericBinary(PO::Description("Generate a generic binary"sv)),
        ConfDumpIR(
            PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."sv)),
        ConfInterruptible(PO::Description("Generate a interruptible binary"sv)),
        ConfEnableInstructionCounting(PO::Description(
            "Enable generating code for counting Wasm instructions executed."sv)),
        ConfEnableGasMeasuring(PO::Description(
            "Enable generating code for counting gas burned during execution."sv)),
        ConfEnableTimeMeasuring(PO::Description(
            "Enable generating code for counting time during execution."sv)),
        ConfEnableAllStatistics(PO::Description(
            "Enable generating code for all statistics options include "
            "instruction counting, gas measuring, and execution time."sv)),
        PropOptimizationLevel(
            PO::Description("Optimization level, one of 0, 1, 2, 3, s, z."sv),
            PO::DefaultValue(std::string("2"))) {}

  PO::Option<std::string> WasmName;
  PO::Option<std::string> SoName;
  PO::Option<PO::Toggle> ConfGenericBinary;
  PO::Option<PO::Toggle> ConfDumpIR;
  PO::Option<PO::Toggle> ConfInterruptible;
  PO::Option<PO::Toggle> ConfEnableInstructionCounting;
  PO::Option<PO::Toggle> ConfEnableGasMeasuring;
  PO::Option<PO::Toggle> ConfEnableTimeMeasuring;
  PO::Option<PO::Toggle> ConfEnableAllStatistics;
  PO::Option<std::string> PropOptimizationLevel;

  void addOptions(PO::ArgumentParser &Parser) noexcept {
    Parser.add_option(WasmName)
        .add_option(SoName)
        .add_option("dump"sv, ConfDumpIR)
        .add_option("interruptible"sv, ConfInterruptible)
        .add_option("enable-instruction-count"sv, ConfEnableInstructionCounting)
        .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
        .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
        .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
        .add_option("generic-binary"sv, ConfGenericBinary);
    addProposalOptions(Parser);
    // TODO: Move exception handling option into addProposalOptions after
    // AOT mode of exception handling proposal is ready.
    // Parser.add_option("disable-exception-handling"sv, PropExceptionHandling)
    Parser.add_option("optimize"sv, PropOptimizationLevel);
  }
};

int Compiler(struct DriverCompilerOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
