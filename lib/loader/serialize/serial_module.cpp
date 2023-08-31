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

  // Serialize all sections.
  std::vector<std::pair<uint64_t, std::vector<uint8_t>>> Results;
  for (auto Section : Mod.getCustomSections()) {
    if (auto Res = serializeSection(Section); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    } else {
      Results.push_back(std::make_pair(Section.getStartOffset(), *Res));
    }
  }
  if (auto Res = serializeSection(Mod.getTypeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getTypeSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getImportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getImportSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getFunctionSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getFunctionSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getTableSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getTableSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getMemorySection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getMemorySection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getGlobalSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getGlobalSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getExportSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getExportSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getStartSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getStartSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getElementSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getElementSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getCodeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getCodeSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getDataSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getDataSection().getStartOffset(), *Res));
  }
  if (auto Res = serializeSection(Mod.getDataCountSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  } else {
    Results.push_back(
        std::make_pair(Mod.getDataCountSection().getStartOffset(), *Res));
  }

  // Sort result vectors.
  std::sort(Results.begin(), Results.end(),
            [](auto A, auto B) { return A.first < B.first; });

  for (auto P : Results) {
    OutVec.insert(OutVec.end(), P.second.begin(), P.second.end());
  }
  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
