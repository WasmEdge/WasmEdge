// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCMain.cpp - GC test entry point -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the entry point of the GC test suite.
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"

#include "gtest/gtest.h"

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
