// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/tool.h - Tool entrypoint --------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the entrypoint for the tooling executable.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "plugin/plugin.h"
#include "po/argument_parser.h"
#include <string_view>

namespace WasmEdge {
namespace Driver {

using namespace std::literals;

struct DriverToolOptions {
  DriverToolOptions()
      : SoName(PO::Description("Wasm or so file"sv),
               PO::MetaVar("WASM_OR_SO"sv)),
        Args(PO::Description("Execution arguments"sv), PO::MetaVar("ARG"sv)),
        Reactor(PO::Description(
            "Enable reactor mode. Reactor mode calls `_initialize` if exported."sv)),
        Dir(PO::Description(
                "Binding directories into WASI virtual filesystem. Each "
                "directory can be specified as --dir `host_path`. You can also "
                "map a guest directory to a host directory by --dir "
                "`guest_path:host_path`, where `guest_path` specifies the path "
                "that will correspond to `host_path` for calls like `fopen` in "
                "the guest. The default permission is `readwrite`, however, "
                "you can use --dir `guest_path:host_path:readonly` to make the "
                "mapping directory become a read only mode."sv),
            PO::MetaVar("PREOPEN_DIRS"sv)),
        Env(PO::Description(
                "Environ variables. Each variable can be specified as --env "
                "`NAME=VALUE`."sv),
            PO::MetaVar("ENVS"sv)),
        PropWASM1(PO::Description("Set as WASM 1.0 standard."sv)),
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
        PropExceptionHandling(
            PO::Description("Disable Exception handling proposal"sv)),
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
        PropExceptionHandlingDeprecated(PO::Description(
            "(DEPRECATED) Enable Exception handling proposal. WASM 3.0 "
            "includes this proposal, and this option will be removed in the "
            "future."sv)),
        // TODO: MEMORY64 - enable the option.
        // PropMemory64(PO::Description("Disable Memory64 proposal"sv)),
        PropThreads(PO::Description("Enable Threads proposal"sv)),
        PropComponent(PO::Description(
            "Enable Component Model proposal, this is experimental"sv)),
        PropAll(PO::Description("Enable all features"sv)),
        ConfEnableInstructionCounting(PO::Description(
            "Enable generating code for counting Wasm instructions executed."sv)),
        ConfEnableGasMeasuring(PO::Description(
            "Enable generating code for counting gas burned during execution."sv)),
        ConfEnableTimeMeasuring(PO::Description(
            "Enable generating code for counting time during execution."sv)),
        ConfEnableAllStatistics(PO::Description(
            "Enable generating code for all statistics options include "
            "instruction counting, gas measuring, and execution time"sv)),
        ConfEnableJIT(
            PO::Description("Enable Just-In-Time compiler for running WASM"sv)),
        ConfEnableCoredump(PO::Description(
            "Enable coredump when WebAssembly enters a trap"sv)),
        ConfCoredumpWasmgdb(
            PO::Description("Enable coredump for wasm-gdb to debug"sv)),
        ConfForceInterpreter(
            PO::Description("Forcibly run WASM in interpreter mode."sv)),
        ConfAFUNIX(PO::Description("Enable UNIX domain sockets"sv)),
        TimeLim(
            PO::Description(
                "Limitation of maximum time(in milliseconds) for execution, "
                "default value is 0 for no limitations"sv),
            PO::MetaVar("TIMEOUT"sv), PO::DefaultValue<uint64_t>(0)),
        GasLim(
            PO::Description(
                "Limitation of execution gas. Upper bound can be specified as "
                "--gas-limit `GAS_LIMIT`."sv),
            PO::MetaVar("GAS_LIMIT"sv)),
        MemLim(
            PO::Description(
                "Limitation of pages(as size of 64 KiB) in every memory "
                "instance. Upper bound can be specified as --memory-page-limit "
                "`PAGE_COUNT`."sv),
            PO::MetaVar("PAGE_COUNT"sv)),
        ForbiddenPlugins(PO::Description("List of plugins to ignore."sv),
                         PO::MetaVar("NAMES"sv)) {}

  PO::Option<std::string> SoName;
  PO::List<std::string> Args;
  PO::Option<PO::Toggle> Reactor;
  PO::List<std::string> Dir;
  PO::List<std::string> Env;
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
  PO::Option<PO::Toggle> PropExceptionHandling;
  PO::Option<PO::Toggle> PropTailCallDeprecated;
  PO::Option<PO::Toggle> PropExtendConstDeprecated;
  PO::Option<PO::Toggle> PropFunctionReferenceDeprecated;
  PO::Option<PO::Toggle> PropGCDeprecated;
  PO::Option<PO::Toggle> PropMultiMemDeprecated;
  PO::Option<PO::Toggle> PropRelaxedSIMDDeprecated;
  PO::Option<PO::Toggle> PropExceptionHandlingDeprecated;
  // TODO: MEMORY64 - enable the option.
  // PO::Option<PO::Toggle> PropMemory64;
  PO::Option<PO::Toggle> PropThreads;
  PO::Option<PO::Toggle> PropComponent;
  PO::Option<PO::Toggle> PropAll;
  PO::Option<PO::Toggle> ConfEnableInstructionCounting;
  PO::Option<PO::Toggle> ConfEnableGasMeasuring;
  PO::Option<PO::Toggle> ConfEnableTimeMeasuring;
  PO::Option<PO::Toggle> ConfEnableAllStatistics;
  PO::Option<PO::Toggle> ConfEnableJIT;
  PO::Option<PO::Toggle> ConfEnableCoredump;
  PO::Option<PO::Toggle> ConfCoredumpWasmgdb;
  PO::Option<PO::Toggle> ConfForceInterpreter;
  PO::Option<PO::Toggle> ConfAFUNIX;
  PO::Option<uint64_t> TimeLim;
  PO::List<int> GasLim;
  PO::List<int> MemLim;
  PO::List<std::string> ForbiddenPlugins;

  void add_option(PO::ArgumentParser &Parser) noexcept {

    Parser.add_option(SoName)
        .add_option(Args)
        .add_option("reactor"sv, Reactor)
        .add_option("dir"sv, Dir)
        .add_option("env"sv, Env)
        .add_option("enable-instruction-count"sv, ConfEnableInstructionCounting)
        .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
        .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
        .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
        .add_option("enable-jit"sv, ConfEnableJIT)
        .add_option("enable-coredump"sv, ConfEnableCoredump)
        .add_option("coredump-for-wasmgdb"sv, ConfCoredumpWasmgdb)
        .add_option("force-interpreter"sv, ConfForceInterpreter)
        .add_option("allow-af-unix"sv, ConfAFUNIX)
        .add_option("wasm-1"sv, PropWASM1)
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
        .add_option("disable-exception-handling"sv, PropExceptionHandling)
        .add_option("enable-tail-call"sv, PropTailCallDeprecated)
        .add_option("enable-extended-const"sv, PropExtendConstDeprecated)
        .add_option("enable-function-reference"sv,
                    PropFunctionReferenceDeprecated)
        .add_option("enable-gc"sv, PropGCDeprecated)
        .add_option("enable-multi-memory"sv, PropMultiMemDeprecated)
        .add_option("enable-relaxed-simd"sv, PropRelaxedSIMDDeprecated)
        .add_option("enable-exception-handling"sv,
                    PropExceptionHandlingDeprecated)
        // TODO: MEMORY64 - enable the option.
        // .add_option("disable-memory64"sv, PropMemory64)
        .add_option("enable-threads"sv, PropThreads)
        .add_option("enable-component"sv, PropComponent)
        .add_option("enable-all"sv, PropAll)
        .add_option("time-limit"sv, TimeLim)
        .add_option("gas-limit"sv, GasLim)
        .add_option("memory-page-limit"sv, MemLim)
        .add_option("forbidden-plugin"sv, ForbiddenPlugins);

    Plugin::Plugin::loadFromDefaultPaths();
    Plugin::Plugin::addPluginOptions(Parser);
  }
};

int Tool(struct DriverToolOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
