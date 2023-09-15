// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize limit. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeLimit(const AST::Limit &Lim,
                           std::vector<uint8_t> &OutVec) const noexcept {
  // Limit: 0x00 + min:u32
  //       |0x01 + min:u32 + max:u32
  //       |0x02 + min:u32 (shared)
  //       |0x03 + min:u32 + max:u32 (shared)
  uint8_t Flag = 0;
  if (Lim.isShared()) {
    Flag = 0x02U;
  }
  if (Lim.hasMax()) {
    Flag |= 0x01U;
  }
  if (static_cast<AST::Limit::LimitType>(Flag) ==
      AST::Limit::LimitType::SharedNoMax) {
    if (Conf.hasProposal(Proposal::Threads)) {
      return logSerializeError(ErrCode::Value::SharedMemoryNoMax,
                               ASTNodeAttr::Type_Limit);
    }
    return logSerializeError(ErrCode::Value::IntegerTooLarge,
                             ASTNodeAttr::Type_Limit);
  }
  OutVec.push_back(Flag);
  serializeU32(Lim.getMin(), OutVec);
  if (Lim.hasMax()) {
    serializeU32(Lim.getMax(), OutVec);
  }
  return {};
}

// Serialize function type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::FunctionType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Function type: 0x60 + paramtypes:vec(valtype) + returntypes:vec(valtype).
  // Prefix 0x60.
  OutVec.push_back(0x60U);
  // Param types: vec(valtype).
  serializeU32(static_cast<uint32_t>(Type.getParamTypes().size()), OutVec);
  for (auto &VType : Type.getParamTypes()) {
    if (auto Check = checkValTypeProposals(VType, ASTNodeAttr::Type_Function);
        !Check) {
      return Unexpect(Check);
    }
    OutVec.push_back(static_cast<uint8_t>(VType));
  }
  // Return types: vec(valtype).
  if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) &&
      Type.getReturnTypes().size() > 1) {
    return logNeedProposal(ErrCode::Value::MalformedValType,
                           Proposal::MultiValue, ASTNodeAttr::Type_Function);
  }
  serializeU32(static_cast<uint32_t>(Type.getReturnTypes().size()), OutVec);
  for (auto &VType : Type.getReturnTypes()) {
    if (auto Check = checkValTypeProposals(VType, ASTNodeAttr::Type_Function);
        !Check) {
      return Unexpect(Check);
    }
    OutVec.push_back(static_cast<uint8_t>(VType));
  }
  return {};
}

// Serialize table type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::TableType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Table type: elemtype:valtype + limit.
  if (auto Check =
          checkRefTypeProposals(Type.getRefType(), ASTNodeAttr::Type_Table);
      !Check) {
    return Unexpect(Check);
  }
  OutVec.push_back(static_cast<uint8_t>(Type.getRefType()));
  if (auto Res = serializeLimit(Type.getLimit(), OutVec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Table));
    return Unexpect(Res);
  }
  return {};
}

// Serialize memory type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::MemoryType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Memory type: limit.
  if (auto Res = serializeLimit(Type.getLimit(), OutVec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Memory));
    return Unexpect(Res);
  }
  return {};
}

// Serialize global type. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeType(const AST::GlobalType &Type,
                          std::vector<uint8_t> &OutVec) const noexcept {
  // Global type: valtype + valmut.
  if (auto Check =
          checkValTypeProposals(Type.getValType(), ASTNodeAttr::Type_Global);
      !Check) {
    return Unexpect(Check);
  }
  OutVec.push_back(static_cast<uint8_t>(Type.getValType()));
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
