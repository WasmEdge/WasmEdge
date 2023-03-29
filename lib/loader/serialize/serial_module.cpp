#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize module. See "include/loader/serialize.h".
std::vector<uint8_t> Serializer::serializeModule(const AST::Module &Mod) {
  // TODO: refine ugly code
  std::vector<uint8_t> OutVec;
  // Serialize Magic and Version.
  OutVec.insert(OutVec.end(), Mod.getMagic().begin(), Mod.getMagic().end());
  OutVec.insert(OutVec.end(), Mod.getVersion().begin(), Mod.getVersion().end());

  for (auto Section : Mod.getCustomSections()) {
    auto CustomSection = Serializer::serializeSection(Section);
    OutVec.insert(OutVec.end(), CustomSection.begin(), CustomSection.end());
  }
  auto TypeSection = Serializer::serializeSection(Mod.getTypeSection());
  OutVec.insert(OutVec.end(), TypeSection.begin(), TypeSection.end());
  auto ImportSection = Serializer::serializeSection(Mod.getImportSection());
  OutVec.insert(OutVec.end(), ImportSection.begin(), ImportSection.end());
  auto FunctionSection = Serializer::serializeSection(Mod.getFunctionSection());
  OutVec.insert(OutVec.end(), FunctionSection.begin(), FunctionSection.end());
  auto TableSection = Serializer::serializeSection(Mod.getTableSection());
  OutVec.insert(OutVec.end(), TableSection.begin(), TableSection.end());
  auto MemorySection = Serializer::serializeSection(Mod.getMemorySection());
  OutVec.insert(OutVec.end(), MemorySection.begin(), MemorySection.end());
  auto GlobalSection = Serializer::serializeSection(Mod.getGlobalSection());
  OutVec.insert(OutVec.end(), GlobalSection.begin(), GlobalSection.end());
  auto ExportSection = Serializer::serializeSection(Mod.getExportSection());
  OutVec.insert(OutVec.end(), ExportSection.begin(), ExportSection.end());
  auto StartSection = Serializer::serializeSection(Mod.getStartSection());
  OutVec.insert(OutVec.end(), StartSection.begin(), StartSection.end());
  auto ElementSection = Serializer::serializeSection(Mod.getElementSection());
  OutVec.insert(OutVec.end(), ElementSection.begin(), ElementSection.end());
  auto CodeSection = Serializer::serializeSection(Mod.getCodeSection());
  OutVec.insert(OutVec.end(), CodeSection.begin(), CodeSection.end());
  auto DataSection = Serializer::serializeSection(Mod.getDataSection());
  OutVec.insert(OutVec.end(), DataSection.begin(), DataSection.end());
  auto DataCountSection = Serializer::serializeSection(Mod.getDataCountSection());
  OutVec.insert(OutVec.end(), DataCountSection.begin(), DataCountSection.end());

  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
