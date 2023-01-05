// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifdef WASMEDGE_BUILD_FUZZING
#include "driver/fuzzPO.h"
#include "common/log.h"
#include "common/version.h"
#include "po/argument_parser.h"

#include <cstdio>
#include <iostream>
#include <vector>

#include <boost/algorithm/searching/boyer_moore_horspool.hpp>

namespace WasmEdge {
namespace Driver {

int FuzzPO(const uint8_t *Data, size_t Size) noexcept {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  spdlog::set_level(spdlog::level::info);

  PO::Option<std::string> SoName(PO::Description("Wasm or so file"sv),
                                 PO::MetaVar("WASM_OR_SO"sv));
  PO::List<std::string> Args(PO::Description("Execution arguments"sv),
                             PO::MetaVar("ARG"sv));

  PO::Option<PO::Toggle> Reactor(PO::Description(
      "Enable reactor mode. Reactor mode calls `_initialize` if exported."));

  PO::List<std::string> Dir(
      PO::Description(
          "Binding directories into WASI virtual filesystem. Each directories "
          "can specified as --dir `guest_path:host_path`, where `guest_path` "
          "specifies the path that will correspond to `host_path` for calls "
          "like `fopen` in the guest."sv),
      PO::MetaVar("PREOPEN_DIRS"sv));

  PO::List<std::string> Env(
      PO::Description(
          "Environ variables. Each variable can be specified as --env `NAME=VALUE`."sv),
      PO::MetaVar("ENVS"sv));

  PO::Option<PO::Toggle> PropMutGlobals(
      PO::Description("Disable Import/Export of mutable globals proposal"sv));
  PO::Option<PO::Toggle> PropNonTrapF2IConvs(PO::Description(
      "Disable Non-trapping float-to-int conversions proposal"sv));
  PO::Option<PO::Toggle> PropSignExtendOps(
      PO::Description("Disable Sign-extension operators proposal"sv));
  PO::Option<PO::Toggle> PropMultiValue(
      PO::Description("Disable Multi-value proposal"sv));
  PO::Option<PO::Toggle> PropBulkMemOps(
      PO::Description("Disable Bulk memory operations proposal"sv));
  PO::Option<PO::Toggle> PropRefTypes(
      PO::Description("Disable Reference types proposal"sv));
  PO::Option<PO::Toggle> PropSIMD(PO::Description("Disable SIMD proposal"sv));
  PO::Option<PO::Toggle> PropMultiMem(
      PO::Description("Enable Multiple memories proposal"sv));
  PO::Option<PO::Toggle> PropTailCall(
      PO::Description("Enable Tail-call proposal"sv));
  PO::Option<PO::Toggle> PropExtendConst(
      PO::Description("Enable Extended-const proposal"sv));
  PO::Option<PO::Toggle> PropThreads(
      PO::Description("Enable Threads proposal"sv));
  PO::Option<PO::Toggle> PropAll(PO::Description("Enable all features"sv));

  PO::Option<PO::Toggle> ConfEnableInstructionCounting(PO::Description(
      "Enable generating code for counting Wasm instructions executed."sv));
  PO::Option<PO::Toggle> ConfEnableGasMeasuring(PO::Description(
      "Enable generating code for counting gas burned during execution."sv));
  PO::Option<PO::Toggle> ConfEnableTimeMeasuring(PO::Description(
      "Enable generating code for counting time during execution."sv));
  PO::Option<PO::Toggle> ConfEnableAllStatistics(PO::Description(
      "Enable generating code for all statistics options include instruction counting, gas measuring, and execution time"sv));

  PO::Option<uint64_t> TimeLim(
      PO::Description(
          "Limitation of maximum time(in milliseconds) for execution, default value is 0 for no limitations"sv),
      PO::MetaVar("TIMEOUT"sv), PO::DefaultValue<uint64_t>(0));

  PO::List<int> GasLim(
      PO::Description(
          "Limitation of execution gas. Upper bound can be specified as --gas-limit `GAS_LIMIT`."sv),
      PO::MetaVar("GAS_LIMIT"sv));

  PO::List<int> MemLim(
      PO::Description(
          "Limitation of pages(as size of 64 KiB) in every memory instance. Upper bound can be specified as --memory-page-limit `PAGE_COUNT`."sv),
      PO::MetaVar("PAGE_COUNT"sv));

  PO::List<std::string> ForbiddenPlugins(
      PO::Description("List of plugins to ignore."sv), PO::MetaVar("NAMES"sv));

  auto Parser = PO::ArgumentParser();
  Parser.add_option(SoName)
      .add_option(Args)
      .add_option("reactor"sv, Reactor)
      .add_option("dir"sv, Dir)
      .add_option("env"sv, Env)
      .add_option("enable-instruction-count"sv, ConfEnableInstructionCounting)
      .add_option("enable-gas-measuring"sv, ConfEnableGasMeasuring)
      .add_option("enable-time-measuring"sv, ConfEnableTimeMeasuring)
      .add_option("enable-all-statistics"sv, ConfEnableAllStatistics)
      .add_option("disable-import-export-mut-globals"sv, PropMutGlobals)
      .add_option("disable-non-trap-float-to-int"sv, PropNonTrapF2IConvs)
      .add_option("disable-sign-extension-operators"sv, PropSignExtendOps)
      .add_option("disable-multi-value"sv, PropMultiValue)
      .add_option("disable-bulk-memory"sv, PropBulkMemOps)
      .add_option("disable-reference-types"sv, PropRefTypes)
      .add_option("disable-simd"sv, PropSIMD)
      .add_option("enable-multi-memory"sv, PropMultiMem)
      .add_option("enable-tail-call"sv, PropTailCall)
      .add_option("enable-extended-const"sv, PropExtendConst)
      .add_option("enable-threads"sv, PropThreads)
      .add_option("enable-all"sv, PropAll)
      .add_option("time-limit"sv, TimeLim)
      .add_option("gas-limit"sv, GasLim)
      .add_option("memory-page-limit"sv, MemLim)
      .add_option("forbidden-plugin"sv, ForbiddenPlugins);

  static constexpr const std::array<char, 4> Separator = {'\xde', '\xad',
                                                          '\xbe', '\xef'};
  static const boost::algorithm::boyer_moore_horspool Searcher(
      Separator.begin(), Separator.end());
  Span<const char> RawArgs(reinterpret_cast<const char *>(Data), Size);
  std::vector<std::string> ArgvStr;
  std::vector<const char *> Argv;
  while (!RawArgs.empty()) {
    const auto It = Searcher(RawArgs.begin(), RawArgs.end()).first;
    ArgvStr.emplace_back(RawArgs.begin(), It);
    RawArgs = RawArgs.subspan(std::min<size_t>(
        std::distance(RawArgs.begin(), It) + 4, RawArgs.size()));
  }
  for (const auto &Arg : ArgvStr) {
    Argv.push_back(Arg.c_str());
  }

  std::unique_ptr<std::FILE, decltype(&std::fclose)> Out{
      std::fopen("/dev/null", "w"), std::fclose};
  if (!Parser.parse(Out.get(), Argv.size(), Argv.data())) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    fmt::print(Out.get(), "{} version {}\n"sv, Argv.empty() ? "" : Argv[0],
               kVersionString);
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
#endif
