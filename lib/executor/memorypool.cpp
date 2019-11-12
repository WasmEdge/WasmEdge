#include "executor/memorypool.h"

namespace SSVM {
namespace Executor {

MemoryPool::MemoryPool() {
  for (unsigned int I = 0; I < 128; I++) {
    FrameEntryPool.push_back(std::make_unique<FrameEntry>());
    LabelEntryPool.push_back(std::make_unique<LabelEntry>());
    ValueEntryPool.push_back(std::make_unique<ValueEntry>());
  }
  FrameEntryCnt = 128;
  LabelEntryCnt = 128;
  ValueEntryCnt = 128;
}

/// Get and initialize frame entry.
std::unique_ptr<FrameEntry> MemoryPool::getFrameEntry(
    unsigned int ModuleAddr, unsigned int FrameArity,
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs) {
  std::unique_ptr<FrameEntry> Frame = requestFrameEntryFromPool();
  Frame->InitFrameEntry(ModuleAddr, FrameArity, Args, LocalDefs);
  return Frame;
}

std::unique_ptr<FrameEntry> MemoryPool::getFrameEntry(unsigned int ModuleAddr,
                                                      unsigned int FrameArity) {
  std::unique_ptr<FrameEntry> Frame = requestFrameEntryFromPool();
  Frame->InitFrameEntry(ModuleAddr, FrameArity);
  return Frame;
}

/// Recycle frame entry.
void MemoryPool::recycleFrameEntry(std::unique_ptr<FrameEntry> Frame) {
  FrameEntryPool.push_back(std::move(Frame));
}

std::unique_ptr<FrameEntry> MemoryPool::requestFrameEntryFromPool() {
  if (FrameEntryPool.size() == 0) {
    for (unsigned int I = 0; I < FrameEntryCnt; I++) {
      FrameEntryPool.push_back(std::make_unique<FrameEntry>());
    }
  }
  FrameEntryCnt *= 2;
  std::unique_ptr<FrameEntry> Frame = std::move(FrameEntryPool.back());
  FrameEntryPool.pop_back();
  return Frame;
}

} // namespace Executor
} // namespace SSVM