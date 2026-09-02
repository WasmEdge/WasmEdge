// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p3_clocks.cpp - wasi:clocks@0.3.1 host functions ---===//

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "host/wasi/p3/clocks.h"
#include "hostcall.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>

namespace {

using namespace WasmEdge;
using namespace std::chrono_literals;

using HostTest::call;
using HostTest::u64Of;

TEST(WasiP3Clocks, InstanceNamesAreTheInterfaceIds) {
  EXPECT_EQ(Host::WasiP3::ClocksTypesInstance().getComponentName(),
            "wasi:clocks/types@0.3.1");
  EXPECT_EQ(Host::WasiP3::MonotonicClockInstance().getComponentName(),
            "wasi:clocks/monotonic-clock@0.3.1");
  EXPECT_EQ(Host::WasiP3::SystemClockInstance().getComponentName(),
            "wasi:clocks/system-clock@0.3.1");
}

TEST(WasiP3Clocks, MonotonicClockDeclaresItsInterface) {
  Host::WasiP3::MonotonicClockInstance Clock;
  auto *Now = Clock.findFunction("now");
  ASSERT_NE(Now, nullptr);
  EXPECT_FALSE(Now->getFuncType().isAsync());
  EXPECT_TRUE(Now->getFuncType().getParamList().empty());
  ASSERT_EQ(Now->getFuncType().getResultList().size(), 1u);
  EXPECT_EQ(Now->getFuncType().getResultList()[0].getValType(),
            ComponentValType(ComponentTypeCode::U64));
  auto *WaitFor = Clock.findFunction("wait-for");
  ASSERT_NE(WaitFor, nullptr);
  EXPECT_TRUE(WaitFor->getFuncType().isAsync());
  ASSERT_EQ(WaitFor->getFuncType().getParamList().size(), 1u);
  EXPECT_EQ(WaitFor->getFuncType().getParamList()[0].getLabel(), "how-long");
  EXPECT_TRUE(WaitFor->getFuncType().getResultList().empty());
  auto *WaitUntil = Clock.findFunction("wait-until");
  ASSERT_NE(WaitUntil, nullptr);
  EXPECT_TRUE(WaitUntil->getFuncType().isAsync());
  EXPECT_EQ(WaitUntil->getFuncType().getParamList()[0].getLabel(), "when");
  EXPECT_NE(Clock.findFunction("get-resolution"), nullptr);
  EXPECT_NE(Clock.findType("mark"), nullptr);
}

TEST(WasiP3Clocks, MonotonicNowIsMonotone) {
  Host::WasiP3::MonotonicClockInstance Clock;
  auto First = call(Clock, "now", {});
  auto Second = call(Clock, "now", {});
  ASSERT_TRUE(First);
  ASSERT_TRUE(Second);
  EXPECT_LE(u64Of(First), u64Of(Second));
}

TEST(WasiP3Clocks, MonotonicResolutionIsPositive) {
  Host::WasiP3::MonotonicClockInstance Clock;
  auto Res = call(Clock, "get-resolution", {});
  ASSERT_TRUE(Res);
  EXPECT_GT(u64Of(Res), 0u);
}

TEST(WasiP3Clocks, WaitForSleepsAtLeastTheDuration) {
  Host::WasiP3::MonotonicClockInstance Clock;
  const auto T0 = std::chrono::steady_clock::now();
  auto Res = call(Clock, "wait-for", {ComponentValVariant{uint64_t(20000000)}});
  ASSERT_TRUE(Res);
  EXPECT_TRUE(Res->empty());
  EXPECT_GE(std::chrono::steady_clock::now() - T0, 20ms);
}

TEST(WasiP3Clocks, WaitUntilInThePastReturnsAtOnce) {
  Host::WasiP3::MonotonicClockInstance Clock;
  auto Now = call(Clock, "now", {});
  ASSERT_TRUE(Now);
  const auto T0 = std::chrono::steady_clock::now();
  auto Res = call(Clock, "wait-until", {ComponentValVariant{u64Of(Now) - 1}});
  ASSERT_TRUE(Res);
  EXPECT_LT(std::chrono::steady_clock::now() - T0, 100ms);
}

TEST(WasiP3Clocks, WaitUntilInTheFutureSleeps) {
  Host::WasiP3::MonotonicClockInstance Clock;
  auto Now = call(Clock, "now", {});
  ASSERT_TRUE(Now);
  const uint64_t When = u64Of(Now) + 20000000;
  auto Res = call(Clock, "wait-until", {ComponentValVariant{When}});
  ASSERT_TRUE(Res);
  auto After = call(Clock, "now", {});
  ASSERT_TRUE(After);
  EXPECT_GE(u64Of(After), When);
}

TEST(WasiP3Clocks, SystemNowIsARecentInstant) {
  Host::WasiP3::SystemClockInstance Clock;
  auto *Now = Clock.findFunction("now");
  ASSERT_NE(Now, nullptr);
  ASSERT_EQ(Now->getFuncType().getResultList().size(), 1u);
  const auto *Ty = *Clock.getType(
      Now->getFuncType().getResultList()[0].getValType().getTypeIndex());
  ASSERT_NE(Ty, nullptr);
  ASSERT_TRUE(Ty->getDefValType().isRecordTy());
  EXPECT_EQ(Ty->getDefValType().getRecord().LabelTypes[0].getLabel(),
            "seconds");
  EXPECT_EQ(Ty->getDefValType().getRecord().LabelTypes[1].getLabel(),
            "nanoseconds");
  auto Res = call(Clock, "now", {});
  ASSERT_TRUE(Res);
  const auto &Aggregate = std::get<std::shared_ptr<ValComp>>((*Res)[0]);
  ASSERT_TRUE(Aggregate);
  const auto &R = std::get<RecordVal>(Aggregate->V);
  ASSERT_EQ(R.Fields.size(), 2u);
  // After 2023-11-14T22:13:20Z, and the remainder is below one second.
  EXPECT_GT(std::get<int64_t>(R.Fields[0].second), int64_t(1700000000));
  EXPECT_LT(std::get<uint32_t>(R.Fields[1].second), 1000000000u);
  EXPECT_NE(Clock.findType("instant"), nullptr);
}

TEST(WasiP3Clocks, SystemResolutionIsPositive) {
  Host::WasiP3::SystemClockInstance Clock;
  auto Res = call(Clock, "get-resolution", {});
  ASSERT_TRUE(Res);
  EXPECT_GT(u64Of(Res), 0u);
}

TEST(WasiP3Clocks, TypesInstanceExportsDuration) {
  Host::WasiP3::ClocksTypesInstance Types;
  const auto *Duration = Types.findType("duration");
  ASSERT_NE(Duration, nullptr);
  ASSERT_TRUE(Duration->getDefValType().isPrimValType());
  EXPECT_EQ(Duration->getDefValType().getPrimValType(), PrimValType::U64);
}

} // namespace
