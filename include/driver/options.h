// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/options.h - Shared option definitions -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the shared proposal options base struct used by both
/// the runtime tool and the compiler tool.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "common/configure.h"
#include "po/argument_parser.h"
#include <string_view>

namespace WasmEdge {
namespace Driver {

using namespace std::literals;

struct DriverProposalOptions {
  DriverProposalOptions()
      : PropWASM1(PO::Description("Set as WASM 1.0 standard."sv)),
        PropWASM2(PO::Description("Set as WASM 2.0 standard."sv)),
        PropWASM3(PO::Description("Set as WASM 3.0 standard (default)."sv)),
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
        PropTailCall(PO::Description("Disable Tail-call proposal"sv)),
        PropExtendConst(PO::Description("Disable Extended-const proposal"sv)),
        PropFunctionReference(
            PO::Description("Disable Function Reference proposal"sv)),
        PropGC(PO::Description("Disable GC proposal"sv)),
        PropMultiMem(PO::Description("Disable Multiple memories proposal"sv)),
        PropRelaxedSIMD(PO::Description("Disable Relaxed SIMD proposal"sv)),
        PropTailCallDeprecated(PO::Description(
            "(DEPRECATED) Enable Tail-call proposal. WASM 3.0 includes this "
            "proposal, and this option will be removed in the future."sv)),
        PropExtendConstDeprecated(PO::Description(
            "(DEPRECATED) Enable Extended-const proposal. WASM 3.0 includes "
            "this proposal, and this option will be removed in the future."sv)),
        PropFunctionReferenceDeprecated(PO::Description(
            "(DEPRECATED) Enable Function Reference proposal. WASM 3.0 "
            "includes this proposal, and this option will be removed in the "
            "future."sv)),
        PropGCDeprecated(PO::Description(
            "(DEPRECATED) Enable GC proposal. WASM 3.0 includes this proposal, "
            "and this option will be removed in the future."sv)),
        PropMultiMemDeprecated(PO::Description(
            "(DEPRECATED) Enable Multiple memories proposal. WASM 3.0 includes "
            "this proposal, and this option will be removed in the future."sv)),
        PropRelaxedSIMDDeprecated(PO::Description(
            "(DEPRECATED) Enable Relaxed SIMD proposal. WASM 3.0 includes this "
            "proposal, and this option will be removed in the future."sv)),
        PropMemory64(PO::Description("Disable Memory64 proposal"sv)),
        PropThreads(PO::Description("Enable Threads proposal"sv)),
        PropAll(PO::Description("Enable all features"sv)) {}

  PO::Option<PO::Toggle> PropWASM1;
  PO::Option<PO::Toggle> PropWASM2;
  PO::Option<PO::Toggle> PropWASM3;
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
  PO::Option<PO::Toggle> PropGC;
  PO::Option<PO::Toggle> PropMultiMem;
  PO::Option<PO::Toggle> PropRelaxedSIMD;
  PO::Option<PO::Toggle> PropTailCallDeprecated;
  PO::Option<PO::Toggle> PropExtendConstDeprecated;
  PO::Option<PO::Toggle> PropFunctionReferenceDeprecated;
  PO::Option<PO::Toggle> PropGCDeprecated;
  PO::Option<PO::Toggle> PropMultiMemDeprecated;
  PO::Option<PO::Toggle> PropRelaxedSIMDDeprecated;
  PO::Option<PO::Toggle> PropMemory64;
  PO::Option<PO::Toggle> PropThreads;
  PO::Option<PO::Toggle> PropAll;

  void addProposalOptions(PO::ArgumentParser &Parser) noexcept {
    Parser.add_option("wasm-1"sv, PropWASM1)
        .add_option("wasm-2"sv, PropWASM2)
        .add_option("wasm-3"sv, PropWASM3)
        .add_option("disable-import-export-mut-globals"sv, PropMutGlobals)
        .add_option("disable-non-trap-float-to-int"sv, PropNonTrapF2IConvs)
        .add_option("disable-sign-extension-operators"sv, PropSignExtendOps)
        .add_option("disable-multi-value"sv, PropMultiValue)
        .add_option("disable-bulk-memory"sv, PropBulkMemOps)
        .add_option("disable-reference-types"sv, PropRefTypes)
        .add_option("disable-simd"sv, PropSIMD)
        .add_option("disable-tail-call"sv, PropTailCall)
        .add_option("disable-extended-const"sv, PropExtendConst)
        .add_option("disable-function-reference"sv, PropFunctionReference)
        .add_option("disable-gc"sv, PropGC)
        .add_option("disable-multi-memory"sv, PropMultiMem)
        .add_option("disable-relaxed-simd"sv, PropRelaxedSIMD)
        .add_option("enable-tail-call"sv, PropTailCallDeprecated)
        .add_option("enable-extended-const"sv, PropExtendConstDeprecated)
        .add_option("enable-function-reference"sv,
                    PropFunctionReferenceDeprecated)
        .add_option("enable-gc"sv, PropGCDeprecated)
        .add_option("enable-multi-memory"sv, PropMultiMemDeprecated)
        .add_option("enable-relaxed-simd"sv, PropRelaxedSIMDDeprecated)
        .add_option("disable-memory64"sv, PropMemory64)
        .add_option("enable-threads"sv, PropThreads)
        .add_option("enable-all"sv, PropAll);
  }
};

Configure
createProposalConfigure(const struct DriverProposalOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
