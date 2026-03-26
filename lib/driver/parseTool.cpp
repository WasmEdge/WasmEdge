// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/module.h"
#include "common/configure.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "driver/tool.h"
#include "loader/loader.h"

#include <cstdlib>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Driver {

int ParseTool(struct DriverToolOptions &Opt) noexcept {
  std::ios::sync_with_stdio(false);

  Configure Conf = CreateConfigure(Opt);
  const auto InputPath =
      std::filesystem::absolute(std::filesystem::u8path(Opt.SoName.value()));

  Loader::Loader Loader(Conf);
  auto Res = Loader.parseModule(InputPath);
  if (!Res) {
    spdlog::error("Failed to parse WASM module.");
    return EXIT_FAILURE;
  }
  const auto &Mod = **Res;

  fmt::print("{}:  file format wasm 0x1\n\n", InputPath.filename().string());
  fmt::print("Section Details:\n\n");

  // Type Section
  const auto &Types = Mod.getTypeSection().getContent();
  fmt::print("Type[{}]:\n", Types.size());
  for (uint32_t I = 0; I < Types.size(); I++) {
    const auto &FuncType = Types[I].getCompositeType().getFuncType();
    fmt::print(" - type[{}] (", I);
    const auto &Params = FuncType.getParamTypes();
    for (uint32_t J = 0; J < Params.size(); J++) {
      if (J > 0)
        fmt::print(", ");
      fmt::print("{}", Params[J]);
    }
    fmt::print(") -> ");
    const auto &Returns = FuncType.getReturnTypes();
    if (Returns.empty()) {
      fmt::print("nil");
    } else {
      for (uint32_t J = 0; J < Returns.size(); J++) {
        if (J > 0)
          fmt::print(", ");
        fmt::print("{}", Returns[J]);
      }
    }
    fmt::print("\n");
  }

  // Import Section
  const auto &Imports = Mod.getImportSection().getContent();
  fmt::print("\nImport[{}]:\n", Imports.size());
  for (uint32_t I = 0; I < Imports.size(); I++) {
    fmt::print(" - {}[{}]", Imports[I].getExternalType(), I);

    if (Imports[I].getExternalType() == ExternalType::Function) {
      fmt::print(" sig={}", Imports[I].getExternalFuncTypeIdx());
    }

    fmt::print(" <- {}.{}\n", Imports[I].getModuleName(),
               Imports[I].getExternalName());
  }

  // Function Section
  const auto &Funcs = Mod.getFunctionSection().getContent();
  fmt::print("\nFunction[{}]:\n", Funcs.size());
  uint32_t ImportedFuncCount = 0;
  for (const auto &Imp : Imports) {
    if (Imp.getExternalType() == ExternalType::Function)
      ImportedFuncCount++;
  }
  for (uint32_t I = 0; I < Funcs.size(); I++) {
    fmt::print(" - func[{}] sig={}\n", I + ImportedFuncCount, Funcs[I]);
  }

  // Export Section
  const auto &Exports = Mod.getExportSection().getContent();
  fmt::print("\nExport[{}]:\n", Exports.size());
  for (uint32_t I = 0; I < Exports.size(); I++) {
    fmt::print(" - {}[{}] -> \"{}\"\n", Exports[I].getExternalType(),
               Exports[I].getExternalIndex(), Exports[I].getExternalName());
  }

  // Memory Section
  const auto &Mems = Mod.getMemorySection().getContent();
  fmt::print("\nMemory[{}]:\n", Mems.size());
  for (uint32_t I = 0; I < Mems.size(); I++) {
    const auto &Limit = Mems[I].getLimit();
    fmt::print(" - memory[{}] pages: initial={}", I, Limit.getMin());
    if (Limit.hasMax()) {
      fmt::print(" max={}", Limit.getMax());
    }
    fmt::print("\n");
  }

  // Global Section
  const auto &Globals = Mod.getGlobalSection().getContent();
  fmt::print("\nGlobal[{}]:\n", Globals.size());
  for (uint32_t I = 0; I < Globals.size(); I++) {
    const auto &GT = Globals[I].getGlobalType();
    fmt::print(" - global[{}] {} mutable={}\n", I, GT.getValType(),
               GT.getValMut() == ValMut::Var ? 1 : 0);
  }

  // Code Section
  const auto &Codes = Mod.getCodeSection().getContent();
  fmt::print("\nCode[{}]:\n", Codes.size());
  for (uint32_t I = 0; I < Codes.size(); I++) {
    uint32_t LocalCount = 0;
    for (const auto &Local : Codes[I].getLocals()) {
      LocalCount += Local.first;
    }
    fmt::print(" - func[{}] size={} locals={}\n", I + ImportedFuncCount,
               Codes[I].getSegSize(), LocalCount);
  }

  // Custom Sections
  const auto &Customs = Mod.getCustomSections();
  fmt::print("\nCustom[{}]:\n", Customs.size());
  for (const auto &Custom : Customs) {
    fmt::print(" - name: \"{}\"\n", Custom.getName());
  }

  return EXIT_SUCCESS;
}

} // namespace Driver
} // namespace WasmEdge
