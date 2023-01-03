#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize import description. See "include/loader/serialize.h".
void Serializer::serializeDesc(const AST::ImportDesc &Desc,
                               std::vector<uint8_t> &OutVec) {
  // Import description: modname:vec(byte) + extname:vec(byte) + importdesc
  // Module name: vec(byte).
  serializeU32(Desc.getModuleName().size(), OutVec);
  OutVec.insert(OutVec.end(), Desc.getModuleName().begin(),
                Desc.getModuleName().end());
  // External name: vec(byte).
  serializeU32(Desc.getExternalName().size(), OutVec);
  OutVec.insert(OutVec.end(), Desc.getExternalName().begin(),
                Desc.getExternalName().end());

  // Import Desc: extern_type:byte + content:idx|tabletype|memorytype|globaltype
  OutVec.push_back(static_cast<uint8_t>(Desc.getExternalType()));
  switch (Desc.getExternalType()) {
  case ExternalType::Function:
    serializeU32(Desc.getExternalFuncTypeIdx(), OutVec);
    break;
  case ExternalType::Table:
    serializeType(Desc.getExternalTableType(), OutVec);
    break;
  case ExternalType::Memory:
    serializeType(Desc.getExternalMemoryType(), OutVec);
    break;
  case ExternalType::Global:
    serializeType(Desc.getExternalGlobalType(), OutVec);
    break;
  default:
    assumingUnreachable();
  }
}

// Serialize export description. See "include/loader/serialize.h".
void Serializer::serializeDesc(const AST::ExportDesc &Desc,
                               std::vector<uint8_t> &OutVec) {
  // Export description: extname:vec(byte) + exportdesc
  // External name: vec(byte).
  serializeU32(Desc.getExternalName().size(), OutVec);
  OutVec.insert(OutVec.end(), Desc.getExternalName().begin(),
                Desc.getExternalName().end());
  // Export Desc: extern_type:byte + idx:u32.
  OutVec.push_back(static_cast<uint8_t>(Desc.getExternalType()));
  serializeU32(Desc.getExternalIndex(), OutVec);
}

} // namespace Loader
} // namespace WasmEdge
