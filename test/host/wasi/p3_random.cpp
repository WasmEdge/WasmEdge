// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p3_random.cpp - wasi:random@0.3.1 host functions ---===//

#include "common/component_variant.h"
#include "host/wasi/p3/random.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <tuple>
#include <vector>

namespace {

using namespace WasmEdge;
using HostTest::call;
using HostTest::u64Of;

std::vector<uint8_t>
bytesOf(const Expect<std::vector<ComponentValVariant>> &Res) {
  return Runtime::Component::Wit<std::vector<uint8_t>>::from((*Res)[0]);
}

TEST(WasiP3Random, InstanceNamesAreTheInterfaceIds) {
  auto Insts = Host::WasiP3::makeRandomInstances();
  ASSERT_EQ(Insts.size(), 3u);
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:random/random@0.3.1");
  EXPECT_EQ(Insts[1]->getComponentName(), "wasi:random/insecure@0.3.1");
  EXPECT_EQ(Insts[2]->getComponentName(), "wasi:random/insecure-seed@0.3.1");
}

TEST(WasiP3Random, RandomBytesHonourTheLength) {
  Host::WasiP3::RandomInstance Inst;
  auto *Func = Inst.findFunction("get-random-bytes");
  ASSERT_NE(Func, nullptr);
  EXPECT_EQ(Func->getFuncType().getParamList()[0].getLabel(), "max-len");
  auto Empty =
      call(Inst, "get-random-bytes", {ComponentValVariant{uint64_t(0)}});
  ASSERT_TRUE(Empty);
  EXPECT_TRUE(bytesOf(Empty).empty());
  auto Some =
      call(Inst, "get-random-bytes", {ComponentValVariant{uint64_t(37)}});
  ASSERT_TRUE(Some);
  EXPECT_EQ(bytesOf(Some).size(), 37u);
  auto Other =
      call(Inst, "get-random-bytes", {ComponentValVariant{uint64_t(37)}});
  ASSERT_TRUE(Other);
  EXPECT_NE(bytesOf(Some), bytesOf(Other));
  // A huge request is served in part, which the interface allows.
  auto Huge =
      call(Inst, "get-random-bytes", {ComponentValVariant{uint64_t(1) << 40}});
  ASSERT_TRUE(Huge);
  EXPECT_EQ(bytesOf(Huge).size(), 1u << 20);
}

TEST(WasiP3Random, RandomU64Varies) {
  Host::WasiP3::RandomInstance Inst;
  auto A = call(Inst, "get-random-u64", {});
  auto B = call(Inst, "get-random-u64", {});
  ASSERT_TRUE(A);
  ASSERT_TRUE(B);
  EXPECT_NE(u64Of(A), u64Of(B));
}

TEST(WasiP3Random, InsecureBytesAndU64) {
  Host::WasiP3::InsecureInstance Inst;
  auto Some = call(Inst, "get-insecure-random-bytes",
                   {ComponentValVariant{uint64_t(9)}});
  ASSERT_TRUE(Some);
  EXPECT_EQ(bytesOf(Some).size(), 9u);
  auto A = call(Inst, "get-insecure-random-u64", {});
  auto B = call(Inst, "get-insecure-random-u64", {});
  ASSERT_TRUE(A);
  ASSERT_TRUE(B);
  EXPECT_NE(u64Of(A), u64Of(B));
}

TEST(WasiP3Random, InsecureSeedIsATupleOfTwo) {
  Host::WasiP3::InsecureSeedInstance Inst;
  auto Res = call(Inst, "get-insecure-seed", {});
  ASSERT_TRUE(Res);
  auto Seed =
      Runtime::Component::Wit<std::tuple<uint64_t, uint64_t>>::from((*Res)[0]);
  auto Again = call(Inst, "get-insecure-seed", {});
  ASSERT_TRUE(Again);
  EXPECT_NE(Seed,
            (Runtime::Component::Wit<std::tuple<uint64_t, uint64_t>>::from(
                (*Again)[0])));
}

} // namespace
