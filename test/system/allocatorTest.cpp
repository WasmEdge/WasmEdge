// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/allocator.h"
#include <gtest/gtest.h>

namespace {
TEST(AllocatorTest, AllocateAndRelease) {
  uint64_t PageCount = 1;
  uint8_t *Ptr = WasmEdge::Allocator::allocate(PageCount);
  EXPECT_NE(Ptr, nullptr);
  
  // Test writeability
  Ptr[0] = 42;
  EXPECT_EQ(Ptr[0], 42);

  WasmEdge::Allocator::release(Ptr, PageCount);
}

TEST(AllocatorTest, Resize) {
  uint64_t OldPageCount = 1;
  uint64_t NewPageCount = 2;
  
  uint8_t *Ptr = WasmEdge::Allocator::allocate(OldPageCount);
  EXPECT_NE(Ptr, nullptr);

  // Resize should succeed
  uint8_t *NewPtr = WasmEdge::Allocator::resize(Ptr, OldPageCount, NewPageCount);
  EXPECT_NE(NewPtr, nullptr);
  
  // Test writeability on the newly mapped page
  // A Wasm page is typically 64KiB (65536 bytes)
  const uint64_t PageSize = 65536;
  NewPtr[PageSize + 100] = 99;
  EXPECT_EQ(NewPtr[PageSize + 100], 99);

  WasmEdge::Allocator::release(NewPtr, NewPageCount);
}

TEST(AllocatorTest, ChunkAllocation) {
  uint64_t Size = 1024;
  uint8_t *Ptr = WasmEdge::Allocator::allocate_chunk(Size);
  EXPECT_NE(Ptr, nullptr);

  // Default chunks are usually readable/writable
  Ptr[0] = 7;
  EXPECT_EQ(Ptr[0], 7);

  // Change permissions
  EXPECT_TRUE(WasmEdge::Allocator::set_chunk_readable(Ptr, Size));
  // Note: we don't write here as it might cause segfault on some platforms if it's strictly read-only
  
  EXPECT_TRUE(WasmEdge::Allocator::set_chunk_readable_writable(Ptr, Size));
  Ptr[1] = 8;
  EXPECT_EQ(Ptr[1], 8);

  EXPECT_TRUE(WasmEdge::Allocator::set_chunk_executable(Ptr, Size));

  WasmEdge::Allocator::release_chunk(Ptr, Size);
}
} // namespace
