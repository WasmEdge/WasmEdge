#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize module. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeModule(const AST::Module &Mod) {
  std::vector<uint8_t> OutVec;
  // Serialize Magic and Version.
  OutVec.insert(OutVec.end(), Mod.getMagic().begin(), Mod.getMagic().end());
  OutVec.insert(OutVec.end(), Mod.getVersion().begin(), Mod.getVersion().end());

  for (auto Section : Mod.getCustomSections()) {
    if (auto Res = serializeSection(Section); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    } else {
      OutVec.insert(OutVec.end(), Res->begin(), Res->end());
    }
  }
  if (auto Res = serializeSection(Mod.getTypeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getImportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getFunctionSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getTableSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getMemorySection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getGlobalSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getExportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getStartSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getElementSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getCodeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getDataSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  if (auto Res = serializeSection(Mod.getDataCountSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }

  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
