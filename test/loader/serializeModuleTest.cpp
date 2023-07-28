/// Unit tests of serialize modules

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

  WasmEdge::AST::Module Module;

  Module.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Module.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};
  Output = *Ser.serializeModule(Module);
  Expected = {
      0x00U, 0x61U, 0x73U, 0x6DU,                      // Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      // Version
  };
  EXPECT_EQ(Output, Expected);
}
} // namespace
