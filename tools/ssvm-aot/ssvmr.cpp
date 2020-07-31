// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "host/wasi/wasimodule.h"
#include "po/argument_parser.h"
#include "support/filesystem.h"
#include "vm/configure.h"
#include "vm/vm.h"
#include <cstdlib>
#include <iostream>

int main(int Argc, const char *Argv[]) {
  namespace PO = SSVM::PO;
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  SSVM::Log::setErrorLoggingLevel();

  PO::Option<std::string> SoName(PO::Description("Wasm so file"s),
                                 PO::MetaVar("WASM_SO"s));
  PO::List<std::string> Args(PO::Description("Execution arguments"s),
                             PO::MetaVar("ARG"s));

  PO::Option<PO::Toggle> Reactor(PO::Description(
      "Enable reactor mode. Reactor mode calls `_initialize` if exported."));

  PO::List<std::string> Dir(
      PO::Description(
          "Binding directories into WASI virtual filesystem. Each directories "
          "can specified as --dir `host_path:guest_path`, where `guest_path` "
          "specifies the path that will correspond to `host_path` for calls "
          "like `fopen` in the guest."s),
      PO::MetaVar("PREOPEN_DIRS"s));

  PO::List<std::string> Env(
      PO::Description(
          "Environ variables. Each variables can specified as --env `NAME=VALUE`."s),
      PO::MetaVar("ENVS"s));

  if (!PO::ArgumentParser()
           .add_option(SoName)
           .add_option(Args)
           .add_option("reactor", Reactor)
           .add_option("dir", Dir)
           .add_option("env", Env)
           .parse(Argc, Argv)) {
    return 0;
  }

  std::string InputPath = std::filesystem::absolute(SoName.value()).string();
  SSVM::VM::Configure Conf;
  Conf.addVMType(SSVM::VM::Configure::VMType::Wasi);
  Conf.addVMType(SSVM::VM::Configure::VMType::SSVM_Process);
  SSVM::VM::VM VM(Conf);

  SSVM::Host::WasiModule *WasiMod = dynamic_cast<SSVM::Host::WasiModule *>(
      VM.getImportModule(SSVM::VM::Configure::VMType::Wasi));

  WasiMod->getEnv().init(Dir.value(), SoName.value(), Args.value(),
                         Env.value());

  if (!Reactor.value()) {
    // command mode
    if (auto Result = VM.runWasmFile(InputPath, "_start")) {
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
    if (auto Result = VM.loadWasm(InputPath); !Result) {
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
    SSVM::Runtime::Instance::FType FuncType;

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

    std::vector<SSVM::ValVariant> FuncArgs;
    for (size_t I = 0;
         I < FuncType.Params.size() && I + 1 < Args.value().size(); ++I) {
      switch (FuncType.Params[I]) {
      case SSVM::ValType::I32: {
        const uint32_t Value = std::stoll(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        break;
      }
      case SSVM::ValType::I64: {
        const uint64_t Value = std::stoll(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        break;
      }
      case SSVM::ValType::F32: {
        const float Value = std::stod(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
        break;
      }
      case SSVM::ValType::F64: {
        const double Value = std::stod(Args.value()[I + 1]);
        FuncArgs.emplace_back(Value);
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
      }
    }

    if (auto Result = VM.execute(FuncName, FuncArgs)) {
      /// Print results.
      for (size_t I = 0; I < FuncType.Returns.size(); ++I) {
        switch (FuncType.Returns[I]) {
        case SSVM::ValType::I32:
          std::cout << std::get<uint32_t>((*Result)[I]) << '\n';
          break;
        case SSVM::ValType::I64:
          std::cout << std::get<uint64_t>((*Result)[I]) << '\n';
          break;
        case SSVM::ValType::F32:
          std::cout << std::get<float>((*Result)[I]) << '\n';
          break;
        case SSVM::ValType::F64:
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
