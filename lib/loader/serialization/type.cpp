#include "ast/module.h"
#include "loader/loader.h"

namespace WasmEdge {
namespace Serialize {

std::vector<uint8_t> encodeU32(uint32_t Num) {
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

std::vector<uint8_t> serializeFunctionType(AST::FunctionType &FuncType) {

  std::vector<uint8_t> result;
  std::vector<uint8_t> paramTypesSize =
      encodeU32(FuncType.getParamTypes.size());
  std::vector<uint8_t> ReturnTypesSize =
      encodeU32(FuncType.getReturnTypes.size());
  result.push_back(0x60U);
  result.insert(result.end(), paramTypesSize.begin(), paramTypesSize.end());
  result.insert(result.end(), FuncType.getParamTypes().begin(),
                FuncType.getParamTypes().end());
  result.insert(result.end(), ReturnTypesSize.begin(), ReturnTypesSize.end());
  result.insert(result.end(), FuncType.getReturnTypes().begin(),
                FuncType.getReturnTypes().end());

  return result;
}

std::vector<uint8_t> serializeTableType(AST::TableType &TabType) {

  std::vector<uint8_t> result;
  result.push_back(TabType.getRefType());
  std::vector<uint8_t> serializedMin = encodeU32(TabType.getLimit().getMin());

  if (TabType.getLimit().getMax()) // check whether a maximum is present.
  {
    result.push_back(0x01);
    result.insert(result.end(), serializedMin.begin(), serializedMin.end());
    std::vector<uint8_t> serializedMax = encodeU32(
        TabType.getLimit().getMax()); //  U32 size of the contents in bytes
    result.insert(result.end(), serializedMax.begin(), serializedMax.end());

  } else {
    result.push_back(0x00);
    result.insert(result.end(), serializedMin.begin(), serializedMin.end());
  }
  return result;
}

std::vector<uint8_t> serializeMemType(AST::MemoryType &MemType) {

  std::vector<uint8_t> serializedMin = encodeU32(MemType.getLimit().getMin());
  std::vector<uint8_t> result;
  if (MemType.getLimit().getMax()) {
    result.push_back(0x01);
    std::vector<uint8_t> serializedMax = encodeU32(MemType.getLimit().getMax());
    result.insert(result.end(), serializedMax.begin(), serializedMax.end());
    result.insert(result.end(), serializedMin.begin(), serializedMin.end())
  } else {
    funtion_data.push_back(0x00);
    result.insert(result.end(), serializedMin.begin(), serializedMin.end())
  }
  return result;
}

std::vector<uint8_t> serializeGlobType(AST::GlobalSegment &GlobalSegment) {
  std::vector<uint8_t> result;
  result.push_back(GlobalSegment.getGlobalType().getValType());
  result.push_back(GlobalSegment.getGlobalType().getValMut());
  // instruction sequence
  result.insert(result.end(), GlobalSegment.getExpr().getInstrs().begin(),
                GlobalSegment.getExpr().getInstrs().end());
  // terminated with an explicit 0X0B  opcode
  result.push_back(0x0B);

  return result;
}

std::vector<uint8_t> serializeCodeSec(AST::CodeSegment &CodeSegment) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> segSize = encodeU32(CodeSegment.getSegSize());
  result.insert(result.end(), segSize.begin(), segSize.end());

  std::vector<uint8_t> serializedLocalsSize =
      encodeU32(CodeSegment.getLocals().size());
  result.insert(result.end(), serializedLocalsSize.begin(),
                serializedLocalsSize.end());

  std::vector<uint8_t> serializedLocals =
      serializeLocals(CodeSegment.getLocals());
  result.insert(result.end(), serializedLocals.begin(), serializedLocals.end());

  result.insert(result.end(), CodeSegment.getExpr().getInstrs().begin(),
                CodeSegment.getExpr().getInstrs().end());
  result.push_back(0x0B);
  return result;
}

std::vector<uint8_t>
serializeLocals(std::vector<std::pair<uint32_t, ValType>> Locals) {
  std::vector<uint8_t> result;
  for (auto &a : Locals) {
    std::vector<uint8_t> serializedFirst = encodeU32(a.first);
    result.insert(result.end(), serializedFirst.begin(), serializedFirst.end());
    result.push_back(a.second);
  }
  return result;
}

std::vector<uint8_t> serializeDataSegment(AST::DataSegment &DataSegment) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> dataSize = encodeU32(DataSegment.getData());
  if (DataSegment.getMode() == AST::DataSegment::DataMode::Active) {
    if (DataSegment.getIdx()) {
      result.push(0x02);
      result.push(DataSegment.getIdx());
      result.insert(result.end(), DataSegment.getExpr().getInstrs().begin(),
                    DataSegment.getExpr().getInstrs().end());
      result.push_back(0x0B);
      result.insert(result.end(), dataSize.begin(), dataSize.end());
      result.insert(result.end(), DataSegment.getData().begin(),
                    DataSegment.getData().end());

    } else {
      result.push(0x00);
      result.insert(result.end(), DataSegment.getExpr().getInstrs().begin(),
                    DataSegment.getExpr().getInstrs().begin());
      result.push_back(0x0B);
      result.insert(result.end(), dataSize.begin(), dataSize.end());
      result.insert(result.end(), DataSegment.getData().begin(),
                    DataSegment.getData().end());
    }
  } else {
    result.push(0x01);
    result.insert(result.end(), dataSize.begin(), dataSize.end());
    result.insert(result.end(), DataSegment.getData().begin(),
                  DataSegment.getData().end());
  }
  return result;
}

// Need fix
std::vector<uint8_t> serializeElemSec(AST::ElementSegment &ElemSeg) {

  std::vector<uint8_t> result;

  if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Active) {
    if (ElemSeg.getIdx()) {
      if (ElemSeg.getRefType()) {
        result.push_back(00x6);
        std::vector<uint8_t> serializedindex = encodeU32(ElemSeg.getIdx());
        result.insert(result.end(), serializedindex.begin(),
                      serializedindex.end());
        result.insert(
            result.end(),
            ElemSeg.getExpr().getInstrs().begin(), // instruction sequence
            ElemSeg.getExpr().getInstrs().end());
        result.push_back(0x0B);
        result.push_back(ElemSeg.getRefType());
        // append vect(expr)
      } else {
        result.push_back(00x2);
        std::vector<uint8_t> serializedindex = encodeU32(ElemSeg.getIdx());
        result.insert(result.end(), serializedindex.begin(),
                      serializedindex.end());
        result.insert(
            result.end(),
            ElemSeg.getExpr().getInstrs().begin(), // instruction sequence
            ElemSeg.getExpr().getInstrs().end());
        result.push_back(0x0B);
        result.push_back(0x00);
        // append vect(funcidx)
      }
    } else {
      // needs fix on correct condition
      if (ElemSeg.getRefType()) {
        result.push_back(00x0);
        result.insert(result.end(), ElemSeg.getExpr().getInstrs().begin(),
                      ElemSeg.getExpr().getInstrs().end());
        result.push_back(0x0B);
        // append vect(funcidx)
      } else {
        result.push_back(00x4);
        result.insert(result.end(), ElemSeg.getExpr().getInstrs().begin(),
                      ElemSeg.getExpr().getInstrs().end());
        result.push_back(0x0B);
        // append vect(expr)
      }
    }
  } else if (ElemSeg.getMode() == AST::ElementSegment::ElemMode::Passive) {
    if (ElemSeg.getRefType()) {
      result.push_back(00x5);
      result.push_back(ElemSeg.getRefType());
      // append vect(expr)

    } else {
      result.push_back(00x1);
      result.push_back(00x0);
      // append vect(funcidx)
    }
  } else {
    if (ElemSeg.getRefType()) {
      result.push_back(00x3);
      result.push_back(00x0);
      // append vect(funcidx)

    } else {
      result.push_back(00x7);
      result.push_back(ElemSeg.getRefType());
      // append vect(expr)
    }
  }
}

} // namespace Serialize
} // namespace WasmEdge