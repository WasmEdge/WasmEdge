// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

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
} // namespace
