//===-- ssvm/executor/entrymgr.h - Entry memory pool class ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the memory pool class of entries.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "entry/frame.h"
#include "entry/label.h"
#include "entry/value.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

class MemoryPool {
public:
  MemoryPool();
  ~MemoryPool() = default;

  /// Get and initialize frame entry.
  std::unique_ptr<FrameEntry> getFrameEntry(
      unsigned int ModuleAddr, unsigned int FrameArity,
      std::vector<std::unique_ptr<ValueEntry>> &Args,
      const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs);
  std::unique_ptr<FrameEntry> getFrameEntry(unsigned int ModuleAddr,
                                            unsigned int FrameArity);

  /// Recycle frame entry.
  void recycleFrameEntry(std::unique_ptr<FrameEntry> Frame);

private:
  unsigned int FrameEntryCnt;
  unsigned int LabelEntryCnt;
  unsigned int ValueEntryCnt;
  std::vector<std::unique_ptr<FrameEntry>> FrameEntryPool;
  std::vector<std::unique_ptr<LabelEntry>> LabelEntryPool;
  std::vector<std::unique_ptr<ValueEntry>> ValueEntryPool;
  std::unique_ptr<FrameEntry> requestFrameEntryFromPool();
};

} // namespace Executor
} // namespace SSVM