#include "ast/module.h"
#include "loader/loader.h"

namespace WasmEdge {
namespace Serialize {

std::vector<uint8_t> serializeImportDesc(AST::ImportDesc &ImpDesc) {
  std::vector<uint8_t> result;
  result.insert(result.end(), ImpDesc.getModuleName().begin(),
                ImpDesc.getModuleName().end());
  result.insert(result.end(), ImpDesc.getExternalName().begin(),
                ImpDesc.getExternalName().end());

  switch (ImpDesc.getExternalType()) {

  case ExternalType::Function: {
    result.push_back(0x00U);
    std::vector<uint8_t> externalFuncType =
        encodeU32(ImpDesc.getExternalFuncTypeIdx());
    result.insert(result.end(), externalFuncType.begin(),
                  externalFuncType.end());
    break;
  }
  case ExternalType::Table: {
    result.push_back(0x01U);
    std::vector<uint8_t> externaltabletype =
        serializeTableType(ImpDesc.getExternalTableType());
    result.insert(result.end(), externaltabletype.begin(),
                  externaltabletype.end());
    break;
  }
  case ExternalType::Memory: {

    result.push_back(0x02U);
    std::vector<uint8_t> externalmemorytype =
        serializeMemType(ImpDesc.getExternalMemoryType());
    result.insert(result.end(), externalmemorytype.begin(),
                  externalmemorytype.end());
    break;
  }
  case ExternalType::Global: {

    result.push_back(0x03U);
    std::vector<uint8_t> externalglobaltype =
        serializeGlobType(ImpDesc.getExternalGlobalType());
    result.insert(result.end(), ImpDesc.getExternalGlobalType().begin(),
                  ImpDesc.getExternalGlobalType().end());
    break;
  }
  default:
    break;
  }
  return result;
}

std::vector<uint8_t> serializeExportSec(AST::ExportDesc &ExpDesc) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> serializedExternalNameSize =
      encodeU32(ExpDesc.getExternalName().size());
  result.insert(result.end(), serializedExternalNameSize.begin(),
                serializedExternalNameSize.end());
  result.insert(result.end(), ExpDesc.ExternalName().begin(),
                ExpDesc.getExternalName().end());

  std::vector<uint8_t> serializedExternalIndex =
      encodeU32(ExpDesc.getExternalIndex());

  switch (ExpDesc.getExternalType()) {
  case ExternalType::Function: {
    result.push_back(0x00U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
    break;
  }
  case ExternalType::Table: {
    result.push_back(0x01U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
    break;
  }
  case ExternalType::Memory: {

    result.push_back(0x02U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
    break;
  }
  case ExternalType::Global: {

    result.push_back(0x03U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
    break;
  }
  default:
    break;
  }
  return result;
}

} // namespace Serialize
} // namespace WasmEdge