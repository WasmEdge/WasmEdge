#include "ast/module.h"
#include "common/errinfo.h"
#include "common/log.h"
#include "loader/loader.h"

namespace WasmEdge {
namespace Serialize {

std::vector<uint8_t> EncodeU32(uint32_t d) {
  std::vector<uint8_t> encoded;
  // unsigned LEB128 encoding
  do {
    auto x = d & 0b01111111;
    d >>= 7;
    if (d)
      x |= 0b10000000;
    encoded.push_back(x);
  } while (d);
  return encoded;
}

std::vector<uint8_t> serializeFunctionType(AST::FunctionType &FuncType) {

  std::vector<uint8_t> result;
  std::vector<uint8_t> paramTypesSize =
      EncodeU32(FuncType.getParamTypes.size());
  std::vector<uint8_t> ReturnTypesSize =
      EncodeU32(FuncType.getReturnTypes.size());
  result.push_back(0x60U);
  result.insert(result.end(), paramTypesSize.begin(), paramTypesSize.end());
  result.insert(result.end(), FuncType.getParamTypes().begin(),
                FuncType.getParamTypes().end());
  result.insert(result.end(), ReturnTypesSize.begin(), ReturnTypesSize.end());
  result.insert(result.end(), FuncType.getReturnTypes().begin(),
                FuncType.getReturnTypes().end());

  return result;
}

std::vector<uint8_t> serializeImportDesc(const AST::ImportDesc &ImpDesc) {
  std::vector<uint8_t> result;
  result.insert(result.end(), ImpDesc.getModuleName().begin(),
                ImpDesc.getModuleName().end());
  result.insert(result.end(), ImpDesc.getExternalName().begin(),
                ImpDesc.getExternalName().end());

  switch (ImpDesc.getExternalType()) {

  case ExternalType::Function: {
    result.push_back(0x00U);
    std::vector<uint8_t> externalFuncType =
        EncodeU32(ImpDesc.getExternalFuncTypeIdx());
    result.insert(result.end(), externalFuncType.begin(),
                  externalFuncType.end());
  }
  case ExternalType::Table: {
    result.push_back(0x01U);
    std::vector<uint8_t> externaltabletype =
        serializeTableType(ImpDesc.getExternalTableType());
    result.insert(result.end(), externaltabletype.begin(),
                  externaltabletype.end());
  }
  case ExternalType::Memory: {

    result.push_back(0x02U);
    std::vector<uint8_t> externalmemorytype =
        serializeMemType(ImpDesc.getExternalMemoryType());
    result.insert(result.end(), externalmemorytype.begin(),
                  externalmemorytype.end());
  }
  case ExternalType::Global: {

    result.push_back(0x03U);
    std::vector<uint8_t> externalglobaltype =
        serializeGlobType(ImpDesc.getExternalGlobalType());
    result.insert(result.end(), ImpDesc.getExternalGlobalType().begin(),
                  ImpDesc.getExternalGlobalType().end());
  }
  }
  return result;
}

std::vector<uint8_t> serializeTableType(AST::TableType &TabType) {

  std::vector<uint8_t> result;
  result.push_back(TabType.getRefType());
  std::vector<uint8_t> serializedMin = EncodeU32(TabType.getLimit().getMin());

  if (TabType.getLimit().getMax()) // check whether a maximum is present.
  {
    result.push_back(0x01);
    result.insert(result.end(), serializedMin.begin(), serializedMin.end());
    std::vector<uint8_t> serializedMax = EncodeU32(
        TabType.getLimit().getMax()); //  U32 size of the contents in bytes
    result.insert(result.end(), serializedMax.begin(), serializedMax.end());

  } else {
    result.push_back(0x00);
    result.insert(result.end(), serializedMin.begin(), serializedMin.end());
  }
  return result;
}

std::vector<uint8_t> serializeMemType(AST::MemoryType &MemType) {

  std::vector<uint8_t> serializedMin = EncodeU32(MemType.getLimit().getMin());
  std::vector<uint8_t> result;
  if (MemType.getLimit().getMax()) {
    result.push_back(0x01);
    std::vector<uint8_t> serializedMax = EncodeU32(MemType.getLimit().getMax());
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
  result.insert(
      result.end(),
      GlobalSegment.getExpr().getInstrs().begin(), // instruction sequence
      GlobalSegment.getExpr().getInstrs().end());
  result.push_back(0x0B); // terminated with an explicit 0X0B  opcode for .

  return result;
}

std::vector<uint8_t> serializeExportSec(const AST::ExportDesc &ExpDesc) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> serializedExternalNameSize =
      EncodeU32(ExpDesc.getExternalName().size());
  result.insert(result.end(), serializedExternalNameSize.begin(),
                serializedExternalNameSize.end());
  result.insert(result.end(), ExpDesc.ExternalName().begin(),
                ExpDesc.getExternalName().end());

  std::vector<uint8_t> serializedExternalIndex =
      EncodeU32(ExpDesc.getExternalIndex());

  switch (ExpDesc.getExternalType()) {
  case ExternalType::Function: {
    result.push_back(0x00U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
  }
  case ExternalType::Table: {
    result.push_back(0x01U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
  }
  case ExternalType::Memory: {

    result.push_back(0x02U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
  }
  case ExternalType::Global: {

    result.push_back(0x03U);
    result.insert(result.end(), serializedExternalIndex.begin(),
                  serializedExternalIndex.end());
  }
  }
  return result;
}

std::vector<uint8_t> serializeCodeSec(AST::CodeSegment &CodeSegment) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> serializedSegSize = EncodeU32(CodeSegment.getSegSize());
  result.insert(result.end(), serializedSegSize.begin(),
                serializedSegSize.end());

  std::vector<uint8_t> serializedLocalsSize =
      EncodeU32(CodeSegment.getLocals().size());
  result.insert(result.end(), serializedLocalsSize.begin(),
                serializedLocalsSize.end());

  std::vector<uint8_t> serializedLocals =
      serializeLocals(CodeSegment.getLocals());
  result.insert(result.end(), erializedLocals.begin(), serializedLocals.end());

  result.insert(result.end(), CodeSegment.getExpr().getInstrs().begin(),
                CodeSegment.getExpr().getInstrs().end());
  result.push_back(0x0B);

  return result;
}

std::vector<uint8_t>
serializeLocals(std::vector<std::pair<uint32_t, ValType>> Locals) {
  std::vector<uint8_t> result;
  for (auto &a : Locals) {
    std::vector<uint8_t> serializedfirst = EncodeU32(a.first);
    result.insert(result.end(), serializedfirst.begin(), serializedfirst.end());
    result.push_back(a.second);
  }
  return result;
}

std::vector<uint8_t> serializeDataSegment(AST::DataSegment &DataSegment) {
  std::vector<uint8_t> result;
  if (DataSegment.getMode() == AST::DataSegment::DataMode::Active) {
    result.push(0);
    result.push(DataSegment.getIdx());
    result.insert(result.end(), DataSegment.getExpr().getInstrs().begin(),
                  DataSegment.getExpr().getInstrs().begin());
    result.insert(result.end(), DataSegment.getData().begin(),  DataSegment.getData().end();
  } else {
    result.push(1);
    result.insert(result.end(), DataSegment.getData(), getData().begin());
  }

  return result;
}

/// Need to fix
std::vector<uint8_t> serializeElemSec(const AST::ElementSegment &ElemSeg) {
  std::vector<uint8_t> result;
  result.push_back(ElemSeg.getRefType());
  std::vector<uint8_t> serializeexpression =
      serializeexpression(getInitExprs());
  result.insert(result.end(), serializeexpression.begin(),
                serializeexpression.end());
  result.insert(result.end(), ElemSeg.getMode(), ElemSeg.getMode());
}

std::vector<uint8_t> serializeexpression(std::vector<Expression> s) {
  std::vector<uint8_t> result;
  for (auto &aa : so) {
    result.push_back(aa);
  }
  return result;
}

} // namespace Serialize
} // namespace WasmEdge