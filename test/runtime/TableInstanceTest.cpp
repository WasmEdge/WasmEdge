// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/runtime/TableInstanceTest.cpp -----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Tests for TableInstance: shrinking and regrowing the element storage.
///
//===----------------------------------------------------------------------===//

#include "common/types.h"
#include "runtime/instance/table.h"

#include <gtest/gtest.h>

namespace {

using namespace WasmEdge;

TEST(TableInstanceTest, ShrinkAndRegrow) {
  Runtime::Instance::TableInstance Table(
      AST::TableType(TypeCode::FuncRef, 1, 3));
  const auto *SizePtr = Table.getSizePtr();
  auto **DataPtr = &Table.getDataPtr();
  ASSERT_TRUE(Table.growTable(2));
  EXPECT_FALSE(Table.shrinkTable(4));
  ASSERT_TRUE(Table.shrinkTable(2));
  EXPECT_EQ(Table.getSize(), 1U);
  EXPECT_EQ(*SizePtr, 1U);
  EXPECT_EQ(DataPtr, &Table.getDataPtr());
  ASSERT_TRUE(Table.growTable(2));
  EXPECT_EQ(*SizePtr, 3U);
  ASSERT_TRUE(Table.shrinkTable(3));
  EXPECT_EQ(*SizePtr, 0U);
  ASSERT_TRUE(Table.growTable(1));
  EXPECT_TRUE(Table.getRefAddr(0)->isNull());
}

} // namespace
