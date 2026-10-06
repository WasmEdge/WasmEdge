// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/driver/compiler.h - Compiler entry point -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the entry point for the compiler executable.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "common/configure.h"
#include "common/span.h"
#include "common/types.h"
#include "driver/options.h"
#include "po/argument_parser.h"
#include <filesystem>
#include <string_view>

namespace WasmEdge {
namespace Driver {

using namespace std::literals;

struct DriverCompilerOptions : public DriverProposalOptions,
                               public DriverStatisticsOptions,
                               public DriverLoggingOptions {
  DriverCompilerOptions()
      : WasmName(PO::Description("Wasm file"sv), PO::MetaVar("WASM"sv)),
        SoName(PO::Description("Wasm so file"sv), PO::MetaVar("WASM_SO"sv)),
        ConfGenericBinary(PO::Description("Generate a generic binary"sv)),
        ConfDumpIR(
            PO::Description("Dump LLVM IR to `wasm.ll` and `wasm-opt.ll`."sv)),
        ConfInterruptible(PO::Description("Generate a interruptible binary"sv)),
        PropOptimizationLevel(
            PO::Description("Optimization level, one of 0, 1, 2, 3, s, z."sv),
            PO::DefaultValue(std::string("2"))) {}

  PO::Option<std::string> WasmName;
  PO::Option<std::string> SoName;
  PO::Option<PO::Toggle> ConfGenericBinary;
  PO::Option<PO::Toggle> ConfDumpIR;
  PO::Option<PO::Toggle> ConfInterruptible;
  PO::Option<std::string> PropOptimizationLevel;

  void addOptions(PO::ArgumentParser &Parser) noexcept {
    addLoggingOptions(Parser);
    Parser.add_option(WasmName)
        .add_option(SoName)
        .add_option("dump"sv, ConfDumpIR)
        .add_option("interruptible"sv, ConfInterruptible);
    addStatisticsOptions(Parser);
    Parser.add_option("generic-binary"sv, ConfGenericBinary);
    addProposalOptions(Parser);
    Parser.add_option("optimize"sv, PropOptimizationLevel);
  }
};

int compileModule(const Configure &Conf, Span<const Byte> Data,
                  const std::filesystem::path &OutputPath) noexcept;

int Compiler(struct DriverCompilerOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
