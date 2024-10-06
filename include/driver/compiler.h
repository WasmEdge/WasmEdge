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
#include "po/argument_parser.h"
#include <string_view>

namespace WasmEdge {
namespace Driver {

using namespace std::literals;

struct DriverCompilerOptions {
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
        ConfEnableAllStatistics(
            PO::Description("Enable generating code for all statistics options "
                            "include instruction "
                            "counting, gas measuring, and execution time"sv)),
        PropMutGlobals(PO::Description(
            "Disable Import/Export of mutable globals proposal"sv)),
        PropNonTrapF2IConvs(PO::Description(
            "Disable Non-trapping float-to-int conversions proposal"sv)),
        PropSignExtendOps(
            PO::Description("Disable Sign-extension operators proposal"sv)),
        PropMultiValue(PO::Description("Disable Multi-value proposal"sv)),
        PropBulkMemOps(
            PO::Description("Disable Bulk memory operations proposal"sv)),
        PropRefTypes(PO::Description("Disable Reference types proposal"sv)),
        PropSIMD(PO::Description("Disable SIMD proposal"sv)),
        PropTailCall(PO::Description("Enable Tail-call proposal"sv)),
        PropExtendConst(PO::Description("Enable Extended-const proposal"sv)),
        PropFunctionReference(
            PO::Description("Enable Function Reference proposal"sv)),
        PropMultiMem(PO::Description("Enable Multiple memories proposal"sv)),
        PropThreads(PO::Description("Enable Threads proposal"sv)),
        PropRelaxedSIMD(PO::Description("Enable Relaxed SIMD proposal"sv)),
        PropAll(PO::Description("Enable all features"sv)),
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
  PO::Option<PO::Toggle> PropMutGlobals;
  PO::Option<PO::Toggle> PropNonTrapF2IConvs;
  PO::Option<PO::Toggle> PropSignExtendOps;
  PO::Option<PO::Toggle> PropMultiValue;
  PO::Option<PO::Toggle> PropBulkMemOps;
  PO::Option<PO::Toggle> PropRefTypes;
  PO::Option<PO::Toggle> PropSIMD;
  PO::Option<PO::Toggle> PropTailCall;
  PO::Option<PO::Toggle> PropExtendConst;
  PO::Option<PO::Toggle> PropFunctionReference;
  PO::Option<PO::Toggle> PropMultiMem;
  PO::Option<PO::Toggle> PropThreads;
  PO::Option<PO::Toggle> PropRelaxedSIMD;
  PO::Option<PO::Toggle> PropAll;
  PO::Option<std::string> PropOptimizationLevel;

  void add_option(PO::ArgumentParser &Parser) noexcept {
    Parser.add_option(WasmName)
        .add_option(SoName)
        .add_option("dump"sv, ConfDumpIR)
        .add_option("interruptible"sv, ConfInterruptible)
        .add_option("enable-instruction-count"sv, ConfEnableInstructionCounting)
        .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
        .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
        .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
        .add_option("generic-binary"sv, ConfGenericBinary)
        .add_option("disable-import-export-mut-globals"sv, PropMutGlobals)
        .add_option("disable-non-trap-float-to-int"sv, PropNonTrapF2IConvs)
        .add_option("disable-sign-extension-operators"sv, PropSignExtendOps)
        .add_option("disable-multi-value"sv, PropMultiValue)
        .add_option("disable-bulk-memory"sv, PropBulkMemOps)
        .add_option("disable-reference-types"sv, PropRefTypes)
        .add_option("disable-simd"sv, PropSIMD)
        .add_option("enable-tail-call"sv, PropTailCall)
        .add_option("enable-extended-const"sv, PropExtendConst)
        .add_option("enable-function-reference"sv, PropFunctionReference)
        .add_option("enable-multi-memory"sv, PropMultiMem)
        .add_option("enable-threads"sv, PropThreads)
        .add_option("enable-relaxed-simd"sv, PropRelaxedSIMD)
        .add_option("enable-all"sv, PropAll)
        .add_option("optimize"sv, PropOptimizationLevel);
  }
};

int Compiler(struct DriverCompilerOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
