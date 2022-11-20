#include "ast/type.h"
#include "ast/module.h"
#include "loader/loader.h"
#include "loader/serialize.h"

namespace WasmEdge {
namespace Serialize {

// Helper functions
std::vector<uint8_t> encodeU32Type(uint32_t Num) {
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

// Serialize Types
std::vector<uint8_t>
Serialize::serializeFunctionType(AST::FunctionType &FuncType) {

  std::vector<uint8_t> SerializeType;
  std::vector<uint8_t> paramTypesSize =
      encodeU32Type(FuncType.getParamTypes().size());
  std::vector<uint8_t> ReturnTypesSize =
      encodeU32Type(FuncType.getReturnTypes().size());

  SerializeType.push_back(0x60U);
  SerializeType.insert(SerializeType.end(), paramTypesSize.begin(),
                       paramTypesSize.end());
  std::transform(FuncType.getParamTypes().begin(),
                 FuncType.getParamTypes().end(),
                 std::back_inserter(SerializeType), [](ValType a) {
                   return static_cast<std::underlying_type_t<ValType>>(a);
                 });

  SerializeType.insert(SerializeType.end(), ReturnTypesSize.begin(),
                       ReturnTypesSize.end());
  std::transform(FuncType.getReturnTypes().begin(),
                 FuncType.getReturnTypes().end(),
                 std::back_inserter(SerializeType), [](ValType b) {
                   return static_cast<std::underlying_type_t<ValType>>(b);
                 });

  return SerializeType;
}

std::vector<uint8_t> Serialize::serializeTableType(AST::TableType &TabType) {

  std::vector<uint8_t> SerializeType;

  SerializeType.push_back(
      static_cast<std::underlying_type_t<RefType>>(TabType.getRefType()));

  std::vector<uint8_t> SerializedMin =
      encodeU32Type(TabType.getLimit().getMin());

  if (TabType.getLimit().getMax()) // check whether a maximum is present.
  {
    SerializeType.push_back(0x01);
    SerializeType.insert(SerializeType.end(), SerializedMin.begin(),
                         SerializedMin.end());
    std::vector<uint8_t> SerializedMax = encodeU32Type(
        TabType.getLimit().getMax()); //  U32 size of the contents in bytes
    SerializeType.insert(SerializeType.end(), SerializedMax.begin(),
                         SerializedMax.end());
  } else {
    SerializeType.push_back(0x00);
    SerializeType.insert(SerializeType.end(), SerializedMin.begin(),
                         SerializedMin.end());
  }
  return SerializeType;
}

std::vector<uint8_t> Serialize::serializeMemType(AST::MemoryType &MemType) {

  std::vector<uint8_t> SerializedMin =
      encodeU32Type(MemType.getLimit().getMin());
  std::vector<uint8_t> SerializeType;
  if (MemType.getLimit().getMax()) {
    SerializeType.push_back(0x01);
    std::vector<uint8_t> SerializedMax =
        encodeU32Type(MemType.getLimit().getMax());
    SerializeType.insert(SerializeType.end(), SerializedMin.begin(),
                         SerializedMin.end());
    SerializeType.insert(SerializeType.end(), SerializedMax.begin(),
                         SerializedMax.end());
  } else {
    SerializeType.push_back(0x00);
    SerializeType.insert(SerializeType.end(), SerializedMin.begin(),
                         SerializedMin.end());
  }
  return SerializeType;
}

std::vector<uint8_t> Serialize::serializeGlobType(AST::GlobalType &GlobType) {

  std::vector<uint8_t> SerializeType;

  SerializeType.push_back(
      static_cast<std::underlying_type_t<ValType>>(GlobType.getValType()));

  SerializeType.push_back(
      static_cast<std::underlying_type_t<ValMut>>(GlobType.getValMut()));

  return SerializeType;
}

} // namespace Serialize
} // namespace WasmEdge