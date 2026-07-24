// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/driver/tool.h - Tool entry point -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the entry point for the tooling executable.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "driver/options.h"
#include "plugin/plugin.h"
#include "po/argument_parser.h"
#include <optional>
#include <string_view>

namespace WasmEdge {
namespace Driver {

using namespace std::literals;

struct DriverToolOptions : public DriverProposalOptions {
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
        PropComponent(PO::Description(
            "Enable Component Model proposal, this is experimental"sv)),
        ConfEnableInstructionCounting(PO::Description(
            "Enable generating code for counting Wasm instructions executed."sv)),
        ConfEnableGasMeasuring(PO::Description(
            "Enable generating code for counting gas burned during execution."sv)),
        ConfEnableTimeMeasuring(PO::Description(
            "Enable generating code for counting time during execution."sv)),
        ConfEnableAllStatistics(PO::Description(
            "Enable generating code for all statistics options include "
            "instruction counting, gas measuring, and execution time"sv)),
        ConfStatsFormat(
            PO::Description(
                "Output format for statistics: human (default) or json"sv),
            PO::DefaultValue<std::string>("human"sv)),
        ConfEnableJIT(
            PO::Description("Enable Just-In-Time compiler for running WASM"sv)),
        ConfEnableCoredump(PO::Description(
            "Enable coredump when WebAssembly enters a trap"sv)),
        ConfCoredumpWasmgdb(
            PO::Description("Enable coredump for wasm-gdb to debug"sv)),
        ConfForceInterpreter(
            PO::Description("Forcibly run WASM in interpreter mode."sv)),
        ConfRunMode(PO::Description("Set execution mode. Valid values: "
                                    "interpreter, jit, aot, lazyjit. "
                                    "Default is interpreter."sv),
                    PO::MetaVar("MODE"sv), PO::DefaultValue(std::string())),
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
        LinkedModules(
            PO::Description(
                "Register additional WASM modules for linking. Each module "
                "can be specified as --module `name:path`, where `name` is "
                "the module name to export and `path` is the WASM file path."sv),
            PO::MetaVar("MODULES"sv)),
        ForbiddenPlugins(PO::Description("List of plugins to ignore."sv),
                         PO::MetaVar("NAMES"sv)),
        LogLevel(
            PO::Description(
                "Set logging level. Valid values: off, trace, debug, info, "
                "warning, error, fatal. Default is info."sv),
            PO::MetaVar("LEVEL"sv), PO::DefaultValue(std::string())) {}

  PO::Option<std::string> SoName;
  PO::List<std::string> Args;
  PO::Option<PO::Toggle> Reactor;
  PO::List<std::string> Dir;
  PO::List<std::string> Env;
  PO::Option<PO::Toggle> PropComponent;
  PO::Option<PO::Toggle> ConfEnableInstructionCounting;
  PO::Option<PO::Toggle> ConfEnableGasMeasuring;
  PO::Option<PO::Toggle> ConfEnableTimeMeasuring;
  PO::Option<PO::Toggle> ConfEnableAllStatistics;
  PO::Option<std::string> ConfStatsFormat;
  PO::Option<PO::Toggle> ConfEnableJIT;
  PO::Option<PO::Toggle> ConfEnableCoredump;
  PO::Option<PO::Toggle> ConfCoredumpWasmgdb;
  PO::Option<PO::Toggle> ConfForceInterpreter;
  PO::Option<std::string> ConfRunMode;
  PO::Option<PO::Toggle> ConfAFUNIX;
  PO::Option<uint64_t> TimeLim;
  PO::List<int> GasLim;
  PO::List<int> MemLim;
  PO::List<std::string> LinkedModules;
  PO::List<std::string> ForbiddenPlugins;
  PO::Option<std::string> LogLevel;

private:
  void addGlobalOptions(PO::ArgumentParser &Parser) noexcept {
    Parser.add_option("log-level"sv, LogLevel)
        .add_option("forbidden-plugin"sv, ForbiddenPlugins);
  }

public:
  void addParserOptions(PO::ArgumentParser &Parser) noexcept {
    addGlobalOptions(Parser);
    addProposalOptions(Parser);
    Parser.add_option("enable-component"sv, PropComponent).add_option(SoName);
  }

  void addLinkerOptions(PO::ArgumentParser &Parser) noexcept {
    addParserOptions(Parser);

    Plugin::Plugin::loadFromDefaultPaths();
    Plugin::Plugin::addPluginOptions(Parser);

    Parser.add_option("dir"sv, Dir)
        .add_option("env"sv, Env)
        .add_option("module"sv, LinkedModules)
        .add_option("memory-page-limit"sv, MemLim);
  }

  void addOptions(PO::ArgumentParser &Parser) noexcept {
    addLinkerOptions(Parser);

    // pure Execution and Profiling flags
    Parser.add_option(Args)
        .add_option("enable-instruction-count"sv, ConfEnableInstructionCounting)
        .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
        .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
        .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
        .add_option("stats-format"sv, ConfStatsFormat)
        .add_option("enable-jit"sv, ConfEnableJIT)
        .add_option("enable-coredump"sv, ConfEnableCoredump)
        .add_option("coredump-for-wasmgdb"sv, ConfCoredumpWasmgdb)
        .add_option("force-interpreter"sv, ConfForceInterpreter)
        .add_option("run-mode"sv, ConfRunMode)
        .add_option("allow-af-unix"sv, ConfAFUNIX)
        .add_option("time-limit"sv, TimeLim)
        .add_option("gas-limit"sv, GasLim)
        .add_option("reactor"sv, Reactor);
  }
};
Configure createConfigure(const struct DriverToolOptions &Opt) noexcept;
std::optional<RunMode> parseRunModeArg(std::string_view S) noexcept;

int Tool(struct DriverToolOptions &Opt) noexcept;
int ParseTool(struct DriverToolOptions &Opt) noexcept;
int ValidateTool(struct DriverToolOptions &Opt) noexcept;
int InstantiateTool(struct DriverToolOptions &Opt) noexcept;

} // namespace Driver
} // namespace WasmEdge
