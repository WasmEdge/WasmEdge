#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize limit. See "include/loader/serialize.h".
void Serializer::serializeLimit(const AST::Limit &Lim,
                                std::vector<uint8_t> &OutVec) {
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
  OutVec.push_back(Flag);
  serializeU32(Lim.getMin(), OutVec);
  if (Lim.hasMax()) {
    serializeU32(Lim.getMax(), OutVec);
  }
}

void Serializer::serializeType(const AST::FunctionType &Type,
                               std::vector<uint8_t> &OutVec) {
  // Function type: 0x60 + paramtypes:vec(valtype) + returntypes:vec(valtype).
  // Prefix 0x60.
  OutVec.push_back(0x60U);
  // Param types: vec(valtype).
  serializeU32(Type.getParamTypes().size(), OutVec);
  std::transform(Type.getParamTypes().cbegin(), Type.getParamTypes().cend(),
                 std::back_inserter(OutVec),
                 [](const ValType &VT) { return static_cast<uint8_t>(VT); });
  // Return types: vec(valtype).
  serializeU32(Type.getReturnTypes().size(), OutVec);
  std::transform(Type.getReturnTypes().cbegin(), Type.getReturnTypes().cend(),
                 std::back_inserter(OutVec),
                 [](const ValType &VT) { return static_cast<uint8_t>(VT); });
}

void Serializer::serializeType(const AST::TableType &Type,
                               std::vector<uint8_t> &OutVec) {
  // Table type: elemtype:valtype + limit.
  OutVec.push_back(static_cast<uint8_t>(Type.getRefType()));
  serializeLimit(Type.getLimit(), OutVec);
}

void Serializer::serializeType(const AST::MemoryType &Type,
                               std::vector<uint8_t> &OutVec) {
  // Memory type: limit.
  serializeLimit(Type.getLimit(), OutVec);
}

void Serializer::serializeType(const AST::GlobalType &Type,
                               std::vector<uint8_t> &OutVec) {
  // Global type: valtype + valmut.
  OutVec.push_back(static_cast<uint8_t>(Type.getValType()));
  OutVec.push_back(static_cast<uint8_t>(Type.getValMut()));
}

} // namespace Loader
} // namespace WasmEdge
