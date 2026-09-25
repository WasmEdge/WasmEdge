// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/mmap.h"
#include <gtest/gtest.h>
#include <fstream>
#include <string>

namespace {
TEST(MMapTest, BasicMapping) {
  // Check if supported first
  if (!WasmEdge::MMap::supported()) {
    GTEST_SKIP() << "MMap not supported on this platform";
  }

  // Create a temporary file
  std::string TmpPath = "test_mmap_tmp.txt";
  {
    std::ofstream Out(TmpPath);
    Out << "HelloWasmEdge";
  }

  {
    WasmEdge::MMap Map(TmpPath);
    void *Addr = Map.address();
    EXPECT_NE(Addr, nullptr);
    
    // Check content
    const char *Content = static_cast<const char *>(Addr);
    EXPECT_EQ(std::string(Content, 13), "HelloWasmEdge");
  }

  // Cleanup
  std::filesystem::remove(TmpPath);
}
} // namespace
