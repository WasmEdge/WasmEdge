#include "gtest/gtest.h"
#include "llvm/jit.h"

using namespace WasmEdge::LLVM;

class JITLibraryNullTest : public ::testing::Test {
protected:
  JITLibrary *jitLibNull;
  void SetUp() override {
    jitLibNull = reinterpret_cast<JITLibrary *>(malloc(sizeof(JITLibrary)));
    memset(jitLibNull, 0, sizeof(JITLibrary));
  }
  void TearDown() override {
    free(jitLibNull);
  }
};

TEST_F(JITLibraryNullTest, GetIntrinsicsNull) {
  auto result = jitLibNull->getIntrinsics();
  EXPECT_FALSE(result.has_value());
}

TEST_F(JITLibraryNullTest, GetTypesNull) {
  auto result = jitLibNull->getTypes(3);
  EXPECT_EQ(result.size(), 3);
  for (const auto &symbol : result) {
    EXPECT_FALSE(symbol.has_value());
  }
}

TEST_F(JITLibraryNullTest, GetCodesNull) {
  auto result = jitLibNull->getCodes(0, 2);
  EXPECT_EQ(result.size(), 2);
  for (const auto &symbol : result) {
    EXPECT_FALSE(symbol.has_value());
  }
}
