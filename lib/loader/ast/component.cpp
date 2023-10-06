// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadUniversalWASM(AST::Module &Mod) {
  bool FallBackInterpreter = false;
  auto Library = std::make_shared<SharedLibrary>();
  if (auto Res = Library->load(Mod.getAOTSection()); unlikely(!Res)) {
    spdlog::error("    AOT section -- library load failed:{} , use "
                  "interpreter mode instead.",
                  Res.error());
    FallBackInterpreter = true;
  }

  // Check the symbols.
  auto FuncTypeSymbols = Library->getTypes<AST::FunctionType::Wrapper>();
  auto CodeSymbols = Library->getCodes<void>();
  auto IntrinsicsSymbol =
      Library->getIntrinsics<const AST::Module::IntrinsicsTable *>();
  auto &FuncTypes = Mod.getTypeSection().getContent();
  auto &CodeSegs = Mod.getCodeSection().getContent();
  if (!FallBackInterpreter &&
      unlikely(FuncTypeSymbols.size() != FuncTypes.size())) {
    spdlog::error("    AOT section -- number of types not matching:{} {}, "
                  "use interpreter mode instead.",
                  FuncTypeSymbols.size(), FuncTypes.size());
    FallBackInterpreter = true;
  }
  if (!FallBackInterpreter && unlikely(CodeSymbols.size() != CodeSegs.size())) {
    spdlog::error("    AOT section -- number of codes not matching:{} {}, "
                  "use interpreter mode instead.",
                  CodeSymbols.size(), CodeSegs.size());
    FallBackInterpreter = true;
  }
  if (!FallBackInterpreter && unlikely(!IntrinsicsSymbol)) {
    spdlog::error("    AOT section -- intrinsics table symbol not found, use "
                  "interpreter mode instead.");
    FallBackInterpreter = true;
  }

  // Set the symbols into the module.
  if (!FallBackInterpreter) {
    for (size_t I = 0; I < FuncTypes.size(); ++I) {
      FuncTypes[I].setSymbol(std::move(FuncTypeSymbols[I]));
    }
    for (size_t I = 0; I < CodeSegs.size(); ++I) {
      CodeSegs[I].setSymbol(std::move(CodeSymbols[I]));
    }
    Mod.setSymbol(std::move(IntrinsicsSymbol));
  } else {
    // Fallback to the interpreter mode case: Re-read the code section.
    WASMType = InputType::WASM;
    FMgr.seek(Mod.getCodeSection().getStartOffset());
    if (auto Res = loadSection(Mod.getCodeSection()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    }
  }
  return {};
}

Expect<void> Loader::loadModuleAOT(AST::AOTSection &AOTSection) {
  // Find and Read the AOT custom section first. Jump the others.
  // This loop is for checking the input is an universal WASM or not.
  // Therefore, if the configure is set as force interpreter mode, skip this.
  while (WASMType != InputType::SharedLibrary) {
    // This loop only overview the custom sections and read the AOT section.
    // For the other general errors, break and handle in the sequentially
    // parsing below.
    uint8_t NewSectionId = 0x00;
    if (auto Res = FMgr.readByte()) {
      NewSectionId = *Res;
    } else {
      break;
    }

    if (NewSectionId == 0x00U) {
      // Load the section size.
      uint32_t ContentSize = 0;
      if (auto Res = FMgr.readU32()) {
        ContentSize = *Res;
      } else {
        break;
      }
      if (ContentSize > FMgr.getRemainSize()) {
        break;
      }

      // Load the section name.
      auto StartOffset = FMgr.getOffset();
      std::string Name;
      if (auto Res = FMgr.readName()) {
        // The UTF-8 failed case will be ignored here.
        Name = std::move(*Res);
      }

      auto ReadSize = FMgr.getOffset() - StartOffset;
      if (ContentSize < ReadSize) {
        // Syntax error of overread. Jump to the next section.
        FMgr.seek(StartOffset + ContentSize);
        continue;
      }

      if (Name == "wasmedge") {
        // Found the AOT section in universal WASM. Load the AOT code.
        // Read the content.
        std::vector<uint8_t> Content;
        if (auto Res = FMgr.readBytes(ContentSize - ReadSize)) {
          Content = std::move(*Res);
        } else {
          break;
        }

        // Load the AOT section.
        FileMgr VecMgr;
        AST::AOTSection NewAOTSection;
        VecMgr.setCode(Content);
        if (auto Res = loadSection(VecMgr, NewAOTSection)) {
          // Also handle the duplicated AOT sections case.
          // If the new AOT section discovered, use the new one.
          WASMType = InputType::UniversalWASM;
          AOTSection = std::move(NewAOTSection);
        } else {
          // If the new AOT section load failed, use the old one or the
          // interpreter mode.
          if (WASMType == InputType::UniversalWASM) {
            spdlog::info(
                "    Load AOT section failed. Use the previous succeeded one.");
          } else {
            spdlog::info(
                "    Load AOT section failed. Use interpreter mode instead.");
          }
        }
      } else {
        // Found other custom sections. Jump to the next section.
        FMgr.seek(StartOffset + ContentSize);
        continue;
      }
    } else {
      if (auto Res = FMgr.jumpContent(); unlikely(!Res)) {
        break;
      }
    }
  }
  return {};
}

Expect<std::variant<AST::Component, AST::Module>> Loader::loadUnit() {
  // component ::= <preamble> s*:<section>* => (component flatten(s*))
  // preamble  ::= <magic> <version> <layer>
  // magic     ::= 0x00 0x61 0x73 0x6D
  // version   ::= 0x0a 0x00
  // layer     ::= 0x01 0x00

  // The combination of version and layer is corresponding to the version of
  // core wasm.
  // The core module has same magic but the different version:
  // 0x01 0x00 0x00 0x00
  auto Magic = FMgr.readBytes(4);
  if (!Magic) {
    return logLoadError(Magic.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
  if (*Magic != WasmMagic) {
    return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  auto Ver = FMgr.readBytes(4);
  if (!Ver) {
    return logLoadError(Ver.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  std::vector<Byte> ModuleVersion = {0x01, 0x00, 0x00, 0x00};
  // spec says 0x0a, but it's actually 0x0d, where cargo component compiled out
  std::vector<Byte> ComponentVersion = {0x0d, 0x00, 0x01, 0x00};
  if (*Ver == ModuleVersion) {
    auto Mod = std::make_unique<AST::Module>();
    Mod->getMagic() = WasmMagic;
    Mod->getVersion() = ModuleVersion;
    if (!Conf.getRuntimeConfigure().isForceInterpreter()) {

      if (auto Res = loadModuleAOT(Mod->getAOTSection()); !Res) {
        return Unexpect(Res);
      }
    }
    // Seek to the position after the binary header.
    FMgr.seek(8);
    if (auto Res = loadModule(*Mod); !Res) {
      return Unexpect(Res);
    }

    // Load library from AOT Section for the universal WASM case.
    // For the force interpreter mode, skip this.
    if (!Conf.getRuntimeConfigure().isForceInterpreter() &&
        WASMType == InputType::UniversalWASM) {
      if (auto Res = loadUniversalWASM(*Mod); !Res) {
        return Unexpect(Res);
      }
    }
    return *Mod;
  } else if (*Ver == ComponentVersion) {
    auto Comp = std::make_unique<AST::Component>();
    Comp->getMagic() = WasmMagic;
    Comp->getVersion() = {0x0a, 0x00};
    Comp->getLayer() = {0x01, 0x00};
    spdlog::error("Component model is not fully parsed yet!");
    return *Comp;
  } else {
    return logLoadError(ErrCode::Value::MalformedVersion, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
}

} // namespace Loader
} // namespace WasmEdge
