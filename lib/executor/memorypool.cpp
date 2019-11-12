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

/// Get and initialize label entry.
std::unique_ptr<LabelEntry>
MemoryPool::getLabelEntry(const unsigned int LabelArity,
                          AST::Instruction *Instr) {
  std::unique_ptr<LabelEntry> Label = requestLabelEntryFromPool();
  Label->InitLabelEntry(LabelArity, Instr);
  return Label;
}

/// Recycle frame entry.
void MemoryPool::recycleFrameEntry(std::unique_ptr<FrameEntry> Frame) {
  FrameEntryPool.push_back(std::move(Frame));
}

/// Recycle label entry.
void MemoryPool::recycleLabelEntry(std::unique_ptr<LabelEntry> Label) {
  LabelEntryPool.push_back(std::move(Label));
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

std::unique_ptr<LabelEntry> MemoryPool::requestLabelEntryFromPool() {
  if (LabelEntryPool.size() == 0) {
    for (unsigned int I = 0; I < LabelEntryCnt; I++) {
      LabelEntryPool.push_back(std::make_unique<LabelEntry>());
    }
  }
  LabelEntryCnt *= 2;
  std::unique_ptr<LabelEntry> Label = std::move(LabelEntryPool.back());
  LabelEntryPool.pop_back();
  return Label;
}

} // namespace Executor
} // namespace SSVM