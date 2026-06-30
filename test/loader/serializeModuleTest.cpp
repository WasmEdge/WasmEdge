// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"
#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

WasmEdge::AST::Module createPopulatedModule() {
  WasmEdge::AST::Module Module;
  Module.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Module.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};

  // The serializer emits sections in ascending start-offset order (see
  // Serializer::serializeModule, which std::sorts sections by getStartOffset()).
  // The ascending offsets below therefore pin the section order that the
  // Expected byte sequences assume: type, function, memory, export, code.

  WasmEdge::AST::FunctionType FuncType;
  FuncType.getReturnTypes() = {WasmEdge::TypeCode::I32};
  Module.getTypeSection().getContent() = {FuncType};
  Module.getTypeSection().setStartOffset(1);

  Module.getFunctionSection().getContent() = {0x00U};
  Module.getFunctionSection().setStartOffset(2);

  WasmEdge::AST::MemoryType MemoryType;
  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);
  MemoryType.getLimit().setMin(1);
  Module.getMemorySection().getContent() = {MemoryType};
  Module.getMemorySection().setStartOffset(3);

  WasmEdge::AST::ExportDesc ExportDesc;
  ExportDesc.setExternalName("main");
  ExportDesc.setExternalType(WasmEdge::ExternalType::Function);
  ExportDesc.setExternalIndex(0x00U);
  Module.getExportSection().getContent() = {ExportDesc};
  Module.getExportSection().setStartOffset(4);

  WasmEdge::AST::Instruction I32Const(WasmEdge::OpCode::I32__const);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  I32Const.setNum(static_cast<uint32_t>(42));
  WasmEdge::AST::CodeSegment CodeSeg;
  CodeSeg.getExpr().getInstrs() = {I32Const, End};
  Module.getCodeSection().getContent() = {CodeSeg};
  Module.getCodeSection().setStartOffset(5);

  return Module;
}

TEST(serializeModuleTest, SerializeModule) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 1. Test serialize module.
  //
  //   1.  Serialize module only with magic and version;
  //   2.  Serialize module with ordered sections.

  WasmEdge::AST::Module Module;

  Module.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Module.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};
  Output = *Ser.serializeModule(Module);
  Expected = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::CustomSection Sec1;
  Sec1.setName("2");
  Sec1.setStartOffset(2);
  WasmEdge::AST::CustomSection Sec2;
  Sec2.setName("3");
  Sec2.setStartOffset(3);
  WasmEdge::AST::CustomSection Sec3;
  Sec3.setName("1");
  Sec3.setStartOffset(1);
  Module.getCustomSections() = {Sec1, Sec2, Sec3};

  Output = *Ser.serializeModule(Module);
  Expected = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x00U, 0x02U, 0x01U, 0x31U, // Sec3
      0x00U, 0x02U, 0x01U, 0x32U, // Sec1
      0x00U, 0x02U, 0x01U, 0x33U, // Sec2
  };
  EXPECT_EQ(Output, Expected);
}

TEST(serializeModuleTest, PopulatedMultiSectionModule) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize module with multiple populated sections.
  //
  //   1.  Serialize module with type, function, memory, export, and code
  //       sections all containing real content.

  WasmEdge::AST::Module Module = createPopulatedModule();

  auto SerRes = Ser.serializeModule(Module);
  ASSERT_TRUE(SerRes);
  Output = *SerRes;
  Expected = {
      0x00U, 0x61U, 0x73U, 0x6DU,                     // Magic
      0x01U, 0x00U, 0x00U, 0x00U,                     // Version
      0x01U, 0x05U, 0x01U, 0x60U, 0x00U, 0x01U, 0x7FU, // Type section
      0x03U, 0x02U, 0x01U, 0x00U,                     // Function section
      0x05U, 0x03U, 0x01U, 0x00U, 0x01U,              // Memory section
      0x07U, 0x08U, 0x01U, 0x04U, 0x6DU, 0x61U, 0x69U,
      0x6EU, 0x00U, 0x00U,                            // Export section
      0x0AU, 0x06U, 0x01U, 0x04U, 0x00U, 0x41U, 0x2AU,
      0x0BU,                                          // Code section
  };
  EXPECT_EQ(Output, Expected);
}

TEST(serializeModuleTest, ModuleRoundTrip) {
  // 3. Test that serialize -> load -> serialize is lossless.
  //
  //   1.  Serialize a module to bytes, parse those bytes back via the Loader,
  //       serialize the loaded module again, and assert both byte sequences
  //       are bit-for-bit identical.

  WasmEdge::AST::Module Module = createPopulatedModule();

  auto SerRes1 = Ser.serializeModule(Module);
  ASSERT_TRUE(SerRes1);
  std::vector<uint8_t> Bytes1 = *SerRes1;

  WasmEdge::Loader::Loader Ldr(Conf);
  auto Res = Ldr.parseModule(Bytes1);
  ASSERT_TRUE(Res);
  auto &ModPtr = *Res;

  auto SerRes2 = Ser.serializeModule(*ModPtr);
  ASSERT_TRUE(SerRes2);
  std::vector<uint8_t> Bytes2 = *SerRes2;
  EXPECT_EQ(Bytes1, Bytes2);
}
} // namespace
