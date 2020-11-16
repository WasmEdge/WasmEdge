// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasibase.h"
#include "host/wasi/wasifunc.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
using namespace std::literals;

TEST(WasiTest, Args) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  {
    SSVM::Host::WasiArgsSizesGet WasiArgsSizesGet(Env);
    SSVM::Host::WasiArgsGet WasiArgsGet(Env);
    std::vector<SSVM::ValVariant> Rets(1);

    // args: test\0
    Env.init({}, "test"s, {}, {});
    EXPECT_TRUE(WasiArgsSizesGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
        Rets));
    EXPECT_EQ(SSVM::retrieveValue<uint32_t>(Rets[0]), UINT32_C(0));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(5));

    EXPECT_TRUE(WasiArgsGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
        Rets));
    EXPECT_EQ(SSVM::retrieveValue<uint32_t>(Rets[0]), UINT32_C(0));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));
    EXPECT_STREQ(MemInst.getPointer<const char *>(8), "test");

    // args: test\0 abc\0
    Env.init({}, "test"s, std::array{"abc"s}, {});
    EXPECT_TRUE(WasiArgsSizesGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
        Rets));
    EXPECT_EQ(SSVM::retrieveValue<uint32_t>(Rets[0]), UINT32_C(0));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(9));

    EXPECT_TRUE(WasiArgsGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
        Rets));
    EXPECT_EQ(SSVM::retrieveValue<uint32_t>(Rets[0]), UINT32_C(0));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(12));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(17));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0));
    EXPECT_STREQ(MemInst.getPointer<const char *>(12), "test");
    EXPECT_STREQ(MemInst.getPointer<const char *>(17), "abc");

    // args: test\0 \0
    Env.init({}, "test"s, std::array{""s}, {});
    EXPECT_TRUE(WasiArgsSizesGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
        Rets));
    EXPECT_EQ(SSVM::retrieveValue<uint32_t>(Rets[0]), UINT32_C(0));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(6));

    EXPECT_TRUE(WasiArgsGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
        Rets));
    EXPECT_EQ(SSVM::retrieveValue<uint32_t>(Rets[0]), UINT32_C(0));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(12));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(17));
    EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0));
    EXPECT_STREQ(MemInst.getPointer<const char *>(12), "test");
    EXPECT_STREQ(MemInst.getPointer<const char *>(17), "");
  }
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
