// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize import description. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeDesc(const AST::ImportDesc &Desc,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Import description: modname:vec(byte) + extname:vec(byte) + importdesc
  // Module name: vec(byte).
  serializeU32(static_cast<uint32_t>(Desc.getModuleName().size()), OutVec);
  OutVec.insert(OutVec.end(), Desc.getModuleName().begin(),
                Desc.getModuleName().end());
  // External name: vec(byte).
  serializeU32(static_cast<uint32_t>(Desc.getExternalName().size()), OutVec);
  OutVec.insert(OutVec.end(), Desc.getExternalName().begin(),
                Desc.getExternalName().end());

  // Import Desc: extern_type:byte + content:idx|tabletype|memorytype|globaltype
  OutVec.push_back(static_cast<uint8_t>(Desc.getExternalType()));
  switch (Desc.getExternalType()) {
  case ExternalType::Function:
    serializeU32(Desc.getExternalFuncTypeIdx(), OutVec);
    break;
  case ExternalType::Table:
    return serializeType(Desc.getExternalTableType(), OutVec);
  case ExternalType::Memory:
    return serializeType(Desc.getExternalMemoryType(), OutVec);
  case ExternalType::Global:
    if (Desc.getExternalGlobalType().getValMut() == ValMut::Var &&
        unlikely(!Conf.hasProposal(Proposal::ImportExportMutGlobals))) {
      return logNeedProposal(ErrCode::Value::InvalidMut,
                             Proposal::ImportExportMutGlobals,
                             ASTNodeAttr::Desc_Import);
    }
    return serializeType(Desc.getExternalGlobalType(), OutVec);
  default:
    return logSerializeError(ErrCode::Value::Unreachable,
                             ASTNodeAttr::Desc_Import);
  }
  return {};
}

// Serialize export description. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeDesc(const AST::ExportDesc &Desc,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Export description: extname:vec(byte) + exportdesc
  // External name: vec(byte).
  serializeU32(static_cast<uint32_t>(Desc.getExternalName().size()), OutVec);
  OutVec.insert(OutVec.end(), Desc.getExternalName().begin(),
                Desc.getExternalName().end());
  // Export Desc: extern_type:byte + idx:u32.
  OutVec.push_back(static_cast<uint8_t>(Desc.getExternalType()));
  serializeU32(Desc.getExternalIndex(), OutVec);
  return {};
}

} // namespace Loader
} // namespace WasmEdge
