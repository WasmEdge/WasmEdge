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

  // Sort sections according to start offset.
  std::vector<std::shared_ptr<AST::Section>> Sections = {
      std::make_shared<AST::TypeSection>(Mod.getTypeSection()),
      std::make_shared<AST::ImportSection>(Mod.getImportSection()),
      std::make_shared<AST::FunctionSection>(Mod.getFunctionSection()),
      std::make_shared<AST::TableSection>(Mod.getTableSection()),
      std::make_shared<AST::MemorySection>(Mod.getMemorySection()),
      std::make_shared<AST::GlobalSection>(Mod.getGlobalSection()),
      std::make_shared<AST::ExportSection>(Mod.getExportSection()),
      std::make_shared<AST::StartSection>(Mod.getStartSection()),
      std::make_shared<AST::ElementSection>(Mod.getElementSection()),
      std::make_shared<AST::CodeSection>(Mod.getCodeSection()),
      std::make_shared<AST::DataSection>(Mod.getDataSection()),
      std::make_shared<AST::DataCountSection>(Mod.getDataCountSection())};
  for (auto CustomSec : Mod.getCustomSections()) {
    Sections.push_back(std::make_shared<AST::CustomSection>(CustomSec));
  }
  std::sort(Sections.begin(), Sections.end(), [](auto A, auto B) {
    return A->getStartOffset() < B->getStartOffset();
  });

  // Serialize sections.
  for (auto Sec : Sections) {
    Expect<std::vector<uint8_t>> Res;
    if (instanceof <AST::TypeSection>(Sec)) {
      Res = serializeSection(*std::dynamic_pointer_cast<AST::TypeSection>(Sec));
    } else if (instanceof <AST::ImportSection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::ImportSection>(Sec));
    } else if (instanceof <AST::FunctionSection>(Sec)) {
      Res = serializeSection(
          *std::dynamic_pointer_cast<AST::FunctionSection>(Sec));
    } else if (instanceof <AST::TableSection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::TableSection>(Sec));
    } else if (instanceof <AST::MemorySection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::MemorySection>(Sec));
    } else if (instanceof <AST::GlobalSection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::GlobalSection>(Sec));
    } else if (instanceof <AST::ExportSection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::ExportSection>(Sec));
    } else if (instanceof <AST::StartSection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::StartSection>(Sec));
    } else if (instanceof <AST::ElementSection>(Sec)) {
      Res = serializeSection(
          *std::dynamic_pointer_cast<AST::ElementSection>(Sec));
    } else if (instanceof <AST::CodeSection>(Sec)) {
      Res = serializeSection(*std::dynamic_pointer_cast<AST::CodeSection>(Sec));
    } else if (instanceof <AST::DataSection>(Sec)) {
      Res = serializeSection(*std::dynamic_pointer_cast<AST::DataSection>(Sec));
    } else if (instanceof <AST::DataCountSection>(Sec)) {
      Res = serializeSection(
          *std::dynamic_pointer_cast<AST::DataCountSection>(Sec));
    } else if (instanceof <AST::CustomSection>(Sec)) {
      Res =
          serializeSection(*std::dynamic_pointer_cast<AST::CustomSection>(Sec));
    } else {
      return logSerializeError(ErrCode::Value::IllegalPath,
                               ASTNodeAttr::Module);
    }
    if (!Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    }
    OutVec.insert(OutVec.end(), Res->begin(), Res->end());
  }
  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
