// SPDX-License-Identifier: Apache-2.0
#include "common/configure.h"
#include "common/filesystem.h"
#include "common/value.h"
#include "common/version.h"
#include "host/wasi/wasimodule.h"
#include "host/wasmedge_process/processmodule.h"
#include "po/argument_parser.h"
#include "vm/vm.h"

#include <cstdlib>
#include <iostream>

int main(int Argc, const char *Argv[]) {
  namespace PO = WasmEdge::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  WasmEdge::Log::setErrorLoggingLevel();

  PO::Option<std::string> SoName(PO::Description("Wasm or so file"sv),
                                 PO::MetaVar("WASM_OR_SO"sv));
  PO::List<std::string> Args(PO::Description("Execution arguments"sv),
                             PO::MetaVar("ARG"sv));

  PO::Option<PO::Toggle> Reactor(PO::Description(
      "Enable reactor mode. Reactor mode calls `_initialize` if exported."));

  PO::List<std::string> Dir(
      PO::Description(
          "Binding directories into WASI virtual filesystem. Each directories "
          "can specified as --dir `host_path:guest_path`, where `guest_path` "
          "specifies the path that will correspond to `host_path` for calls "
          "like `fopen` in the guest."sv),
      PO::MetaVar("PREOPEN_DIRS"sv));

  PO::List<std::string> Env(
      PO::Description(
          "Environ variables. Each variable can be specified as --env `NAME=VALUE`."sv),
      PO::MetaVar("ENVS"sv));

  PO::Option<PO::Toggle> BulkMemoryOperations(
      PO::Description("Disable Bulk-memory operations"sv));
  PO::Option<PO::Toggle> ReferenceTypes(
      PO::Description("Disable Reference types (externref)"sv));
  PO::Option<PO::Toggle> SIMD(PO::Description("Enable SIMD"sv));
  PO::Option<PO::Toggle> All(PO::Description("Enable all features"sv));

  PO::List<int> MemLim(
      PO::Description(
          "Limitation of pages(as size of 64 KiB) in every memory instance. Upper bound can be specified as --memory-page-limit `PAGE_COUNT`."sv),
      PO::MetaVar("PAGE_COUNT"sv));

  PO::List<std::string> AllowCmd(
      PO::Description(
          "Allow commands called from wasmedge_process host functions. Each command can be specified as --allow-command `COMMAND`."sv),
      PO::MetaVar("COMMANDS"sv));
  PO::Option<PO::Toggle> AllowCmdAll(PO::Description(
      "Allow all commands called from wasmedge_process host functions."sv));

  auto Parser = PO::ArgumentParser();
  if (!Parser.add_option(SoName)
           .add_option(Args)
           .add_option("reactor"sv, Reactor)
           .add_option("dir"sv, Dir)
           .add_option("env"sv, Env)
           .add_option("disable-bulk-memory"sv, BulkMemoryOperations)
           .add_option("disable-reference-types"sv, ReferenceTypes)
           .add_option("enable-simd"sv, SIMD)
           .add_option("enable-all"sv, All)
           .add_option("memory-page-limit"sv, MemLim)
           .add_option("allow-command"sv, AllowCmd)
           .add_option("allow-command-all"sv, AllowCmdAll)
           .parse(Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    std::cout << Argv[0] << " version "sv << WasmEdge::kVersionString << '\n';
    return EXIT_SUCCESS;
  }

  WasmEdge::Configure Conf;
  if (BulkMemoryOperations.value()) {
    Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  }
  if (ReferenceTypes.value()) {
    Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  }
  if (SIMD.value()) {
    Conf.addProposal(WasmEdge::Proposal::SIMD);
  }
  if (All.value()) {
    Conf.addProposal(WasmEdge::Proposal::SIMD);
  }
  if (MemLim.value().size() > 0) {
    Conf.setMaxMemoryPage(MemLim.value().back());
  }

  Conf.addHostRegistration(WasmEdge::HostRegistration::Wasi);
  Conf.addHostRegistration(WasmEdge::HostRegistration::WasmEdge_Process);
  const auto InputPath = std::filesystem::absolute(SoName.value());
  WasmEdge::VM::VM VM(Conf);

  WasmEdge::Host::WasiModule *WasiMod =
      dynamic_cast<WasmEdge::Host::WasiModule *>(
          VM.getImportModule(WasmEdge::HostRegistration::Wasi));
  WasmEdge::Host::WasmEdgeProcessModule *ProcMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessModule *>(
          VM.getImportModule(WasmEdge::HostRegistration::WasmEdge_Process));

  if (AllowCmdAll.value()) {
    ProcMod->getEnv().AllowedAll = true;
  }
  for (auto &Str : AllowCmd.value()) {
    ProcMod->getEnv().AllowedCmd.insert(Str);
  }

  WasiMod->getEnv().init(Dir.value(),
                         InputPath.filename().replace_extension("wasm"sv),
                         Args.value(), Env.value());

  if (!Reactor.value()) {
    // command mode
    if (auto Result = VM.runWasmFile(InputPath.u8string(), "_start");
        Result || Result.error() == WasmEdge::ErrCode::Terminated) {
      return WasiMod->getEnv().getExitCode();
    } else {
      return EXIT_FAILURE;
    }
  } else {
    // reactor mode
    if (Args.value().empty()) {
      std::cerr
          << "A function name is required when reactor mode is enabled.\n";
      return EXIT_FAILURE;
    }
    const auto &FuncName = Args.value().front();
    if (auto Result = VM.loadWasm(InputPath.u8string()); !Result) {
      return EXIT_FAILURE;
    }
    if (auto Result = VM.validate(); !Result) {
      return EXIT_FAILURE;
    }
    if (auto Result = VM.instantiate(); !Result) {
      return EXIT_FAILURE;
    }

    using namespace std::literals::string_literals;
    const auto InitFunc = "_initialize"s;

    bool HasInit = false;
    WasmEdge::Runtime::Instance::FType FuncType;

    for (const auto &Func : VM.getFunctionList()) {
      if (Func.first == InitFunc) {
        HasInit = true;
      } else if (Func.first == FuncName) {
        FuncType = Func.second;
      }
    }

    if (HasInit) {
      if (auto Result = VM.execute(InitFunc); !Result) {
        return EXIT_FAILURE;
      }
    }

    std::vector<WasmEdge::ValVariant> FuncArgs;
    std::vector<WasmEdge::ValType> FuncArgTypes;
    for (size_t I = 0;
         I < FuncType.Params.size() && I + 1 < Args.value().size(); ++I) {
      switch (FuncType.Params[I]) {
      case WasmEdge::ValType::I32: {
        const uint32_t Value = std::stol(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::I32);
        break;
      }
      case WasmEdge::ValType::I64: {
        const uint64_t Value = std::stoll(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::I64);
        break;
      }
      case WasmEdge::ValType::F32: {
        const float Value = std::stof(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::F32);
        break;
      }
      case WasmEdge::ValType::F64: {
        const double Value = std::stod(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::F64);
        break;
      }
      /// TODO: FuncRef and ExternRef
      default:
        break;
      }
    }
    if (FuncType.Params.size() + 1 < Args.value().size()) {
      for (size_t I = FuncType.Params.size() + 1; I < Args.value().size();
           ++I) {
        const uint64_t Value = std::stoll(Args.value()[I]);
        FuncArgs.emplace_back(Value);
        FuncArgTypes.emplace_back(WasmEdge::ValType::F64);
      }
    }

    if (auto Result = VM.execute(FuncName, FuncArgs, FuncArgTypes)) {
      /// Print results.
      for (size_t I = 0; I < FuncType.Returns.size(); ++I) {
        switch (FuncType.Returns[I]) {
        case WasmEdge::ValType::I32:
          std::cout << std::get<uint32_t>((*Result)[I]) << '\n';
          break;
        case WasmEdge::ValType::I64:
          std::cout << std::get<uint64_t>((*Result)[I]) << '\n';
          break;
        case WasmEdge::ValType::F32:
          std::cout << std::get<float>((*Result)[I]) << '\n';
          break;
        case WasmEdge::ValType::F64:
          std::cout << std::get<double>((*Result)[I]) << '\n';
          break;
        /// TODO: FuncRef and ExternRef
        default:
          break;
        }
      }
      return EXIT_SUCCESS;
    } else {
      return EXIT_FAILURE;
    }
  }
}
