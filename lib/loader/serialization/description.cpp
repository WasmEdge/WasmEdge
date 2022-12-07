#include "ast/module.h"
#include "loader/loader.h"
#include "loader/serialize.h"

namespace WasmEdge {
namespace Serialize {

// Helper functions
std::vector<uint8_t> encodeU32Desc(uint32_t Num) {
  std::vector<uint8_t> Encoded;
  // unsigned LEB128 encoding
  do {
    auto x = Num & 0b01111111;
    Num >>= 7;
    if (Num)
      x |= 0b10000000;
    Encoded.push_back(x);
  } while (Num);
  return Encoded;
}

// Serialize Descriptions
std::vector<uint8_t> Serialize::serializeImportDesc(AST::ImportDesc &ImpDesc) {
  std::vector<uint8_t> SerializeDesc;
  std::vector<uint8_t> serializedModuleNameSize =
      encodeU32Desc(ImpDesc.getModuleName().size());
  SerializeDesc.insert(SerializeDesc.end(), serializedModuleNameSize.begin(),
                       serializedModuleNameSize.end());
  SerializeDesc.insert(SerializeDesc.end(), ImpDesc.getModuleName().begin(),
                       ImpDesc.getModuleName().end());
  std::vector<uint8_t> serializedExternalNameSize =
      encodeU32Desc(ImpDesc.getExternalName().size());
  SerializeDesc.insert(SerializeDesc.end(), serializedExternalNameSize.begin(),
                       serializedExternalNameSize.end());
  SerializeDesc.insert(SerializeDesc.end(), ImpDesc.getExternalName().begin(),
                       ImpDesc.getExternalName().end());

  switch (ImpDesc.getExternalType()) {

  case ExternalType::Function: {
    SerializeDesc.push_back(0x00U);
    std::vector<uint8_t> SerializeExtFuncType =
        encodeU32Desc(ImpDesc.getExternalFuncTypeIdx());
    SerializeDesc.insert(SerializeDesc.end(), SerializeExtFuncType.begin(),
                         SerializeExtFuncType.end());
    break;
  }
  case ExternalType::Table: {
    SerializeDesc.push_back(0x01U);
    std::vector<uint8_t> SerializeExtTableType =
        serializeTableType(ImpDesc.getExternalTableType());
    SerializeDesc.insert(SerializeDesc.end(), SerializeExtTableType.begin(),
                         SerializeExtTableType.end());
    break;
  }
  case ExternalType::Memory: {

    SerializeDesc.push_back(0x02U);
    std::vector<uint8_t> SerializeExtMemoryType =
        serializeMemType(ImpDesc.getExternalMemoryType());
    SerializeDesc.insert(SerializeDesc.end(), SerializeExtMemoryType.begin(),
                         SerializeExtMemoryType.end());
    break;
  }
  case ExternalType::Global: {

    SerializeDesc.push_back(0x03U);
    std::vector<uint8_t> SerializeExtGlobalType =
        serializeGlobType(ImpDesc.getExternalGlobalType());
    SerializeDesc.insert(SerializeDesc.end(), SerializeExtGlobalType.begin(),
                         SerializeExtGlobalType.end());

    break;
  }
  default:
    break;
  }
  return SerializeDesc;
}

std::vector<uint8_t> Serialize::serializeExportDesc(AST::ExportDesc &ExpDesc) {
  std::vector<uint8_t> SerializeDesc;
  std::vector<uint8_t> serializedExternalNameSize =
      encodeU32Desc(ExpDesc.getExternalName().size());
  SerializeDesc.insert(SerializeDesc.end(), serializedExternalNameSize.begin(),
                       serializedExternalNameSize.end());
  SerializeDesc.insert(SerializeDesc.end(), ExpDesc.getExternalName().begin(),
                       ExpDesc.getExternalName().end());

  std::vector<uint8_t> serializedExternalIndex =
      encodeU32Desc(ExpDesc.getExternalIndex());

  switch (ExpDesc.getExternalType()) {
  case ExternalType::Function: {
    SerializeDesc.push_back(0x00U);
    SerializeDesc.insert(SerializeDesc.end(), serializedExternalIndex.begin(),
                         serializedExternalIndex.end());
    break;
  }
  case ExternalType::Table: {
    SerializeDesc.push_back(0x01U);
    SerializeDesc.insert(SerializeDesc.end(), serializedExternalIndex.begin(),
                         serializedExternalIndex.end());
    break;
  }
  case ExternalType::Memory: {

    SerializeDesc.push_back(0x02U);
    SerializeDesc.insert(SerializeDesc.end(), serializedExternalIndex.begin(),
                         serializedExternalIndex.end());
    break;
  }
  case ExternalType::Global: {

    SerializeDesc.push_back(0x03U);
    SerializeDesc.insert(SerializeDesc.end(), serializedExternalIndex.begin(),
                         serializedExternalIndex.end());
    break;
  }
  default:
    break;
  }
  return SerializeDesc;
}

} // namespace Serialize
} // namespace WasmEdge