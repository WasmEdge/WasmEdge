// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"
#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser;

WasmEdge::AST::Module createPopulatedModule() {
  WasmEdge::AST::Module Module;
  Module.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Module.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};

  // Known sections are emitted in the order the binary format requires:
  // type, function, memory, export, code.

  WasmEdge::AST::FunctionType FuncType;
  FuncType.getReturnTypes() = {WasmEdge::TypeCode::I32};
  Module.getTypeSection().getContent() = {FuncType};

  Module.getFunctionSection().getContent() = {0x00U};

  WasmEdge::AST::MemoryType MemoryType;
  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);
  MemoryType.getLimit().setMin(1);
  Module.getMemorySection().getContent() = {MemoryType};

  WasmEdge::AST::ExportDesc ExportDesc;
  ExportDesc.setExternalName("main");
  ExportDesc.setExternalType(WasmEdge::ExternalType::Function);
  ExportDesc.setExternalIndex(0x00U);
  Module.getExportSection().getContent() = {ExportDesc};

  WasmEdge::AST::Instruction I32Const(WasmEdge::OpCode::I32__const);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  I32Const.setNum(static_cast<uint32_t>(42));
  WasmEdge::AST::CodeSegment CodeSeg;
  CodeSeg.getExpr().getInstrs() = {I32Const, End};
  Module.getCodeSection().getContent() = {CodeSeg};

  return Module;
}

TEST(SerializeModuleTest, SerializeModule) {
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

TEST(SerializeModuleTest, PopulatedMultiSectionModule) {
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
      0x00U, 0x61U, 0x73U, 0x6DU,                      // Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      // Version
      0x01U, 0x05U, 0x01U, 0x60U, 0x00U, 0x01U, 0x7FU, // Type section
      0x03U, 0x02U, 0x01U, 0x00U,                      // Function section
      0x05U, 0x03U, 0x01U, 0x00U, 0x01U,               // Memory section
      0x07U, 0x08U, 0x01U, 0x04U, 0x6DU, 0x61U, 0x69U,
      0x6EU, 0x00U, 0x00U, // Export section
      0x0AU, 0x06U, 0x01U, 0x04U, 0x00U, 0x41U, 0x2AU,
      0x0BU, // Code section
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeModuleTest, HandBuiltSectionOrder) {
  // 3. Test that a module built without the loader is emitted in the section
  //    order the binary format requires.
  //
  //   1.  The tag section (id 13) must precede the global section (id 6).

  WasmEdge::AST::Module Module;
  Module.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Module.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};

  WasmEdge::AST::FunctionType FuncType;
  Module.getTypeSection().getContent() = {FuncType};

  WasmEdge::AST::TagType TagType;
  TagType.setTypeIdx(0);
  Module.getTagSection().getContent() = {TagType};

  WasmEdge::AST::GlobalSegment GlobalSeg;
  GlobalSeg.getGlobalType().setValType(WasmEdge::TypeCode::I32);
  GlobalSeg.getGlobalType().setValMut(WasmEdge::ValMut::Const);
  WasmEdge::AST::Instruction I32Const(WasmEdge::OpCode::I32__const);
  I32Const.setNum(static_cast<uint32_t>(0));
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  GlobalSeg.getExpr().getInstrs() = {I32Const, End};
  Module.getGlobalSection().getContent() = {GlobalSeg};

  auto SerRes = Ser.serializeModule(Module);
  ASSERT_TRUE(SerRes);

  WasmEdge::Loader::Loader Ldr(Conf);
  EXPECT_TRUE(Ldr.parseModule(*SerRes));
}

TEST(SerializeModuleTest, CustomSectionPositionPreserved) {
  // 4. Test that a custom section keeps its position across a load and a
  //    serialize.
  //
  //   1.  A custom section sitting between the type and function sections must
  //       come back out in the same place.

  const std::vector<uint8_t> Input = {
      0x00U, 0x61U, 0x73U, 0x6DU,               // Magic
      0x01U, 0x00U, 0x00U, 0x00U,               // Version
      0x01U, 0x04U, 0x01U, 0x60U, 0x00U, 0x00U, // Type section
      0x00U, 0x03U, 0x02U, 0x68U, 0x69U,        // Custom section "hi"
      0x03U, 0x02U, 0x01U, 0x00U,               // Function section
      0x0AU, 0x04U, 0x01U, 0x02U, 0x00U, 0x0BU  // Code section
  };

  WasmEdge::Loader::Loader Ldr(Conf);
  auto Res = Ldr.parseModule(Input);
  ASSERT_TRUE(Res);

  auto SerRes = Ser.serializeModule(**Res);
  ASSERT_TRUE(SerRes);
  EXPECT_EQ(*SerRes, Input);
}

TEST(SerializeModuleTest, ModuleRoundTrip) {
  // 5. Test that serialize -> load -> serialize is lossless.
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
