#include "executor/memorypool.h"

namespace SSVM {
namespace Executor {

MemoryPool::MemoryPool()
    : FrameEntryCnt(128), LabelEntryCnt(128), ValueEntryCnt(128) {
  for (unsigned int I = 0; I < 128; I++) {
    FrameEntryPool.push_back(std::make_unique<FrameEntry>());
    LabelEntryPool.push_back(std::make_unique<LabelEntry>());
    ValueEntryPool.push_back(std::make_unique<ValueEntry>());
  }
}

/// Get and initialize frame entry.
std::unique_ptr<FrameEntry> MemoryPool::allocFrameEntry(
    unsigned int ModuleAddr, unsigned int FrameArity,
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs) {
  std::unique_ptr<FrameEntry> Frame = requestFrameEntryFromPool();
  Frame->InitFrameEntry(ModuleAddr, FrameArity, Args, LocalDefs);
  return Frame;
}

std::unique_ptr<FrameEntry>
MemoryPool::allocFrameEntry(unsigned int ModuleAddr, unsigned int FrameArity) {
  std::unique_ptr<FrameEntry> Frame = requestFrameEntryFromPool();
  Frame->InitFrameEntry(ModuleAddr, FrameArity);
  return Frame;
}

/// Get and initialize label entry.
std::unique_ptr<LabelEntry>
MemoryPool::allocLabelEntry(const unsigned int LabelArity,
                            AST::BlockControlInstruction *Instr) {
  std::unique_ptr<LabelEntry> Label = requestLabelEntryFromPool();
  Label->InitLabelEntry(LabelArity, Instr);
  return Label;
}

/// Get and initialize value entry.
std::unique_ptr<ValueEntry> MemoryPool::allocValueEntry(const ValueEntry &VE) {
  std::unique_ptr<ValueEntry> Value = requestValueEntryFromPool();
  Value->InitValueEntry(VE);
  return Value;
}

std::unique_ptr<ValueEntry>
MemoryPool::allocValueEntry(const AST::ValType &VT) {
  std::unique_ptr<ValueEntry> Value = requestValueEntryFromPool();
  Value->InitValueEntry(VT);
  return Value;
}

std::unique_ptr<ValueEntry>
MemoryPool::allocValueEntry(const AST::ValType &VT,
                            const AST::ValVariant &Val) {
  std::unique_ptr<ValueEntry> Value = requestValueEntryFromPool();
  Value->InitValueEntry(VT, Val);
  return Value;
}

std::unique_ptr<ValueEntry>
MemoryPool::allocValueEntry(const AST::ValVariant &Val) {
  std::unique_ptr<ValueEntry> Value = requestValueEntryFromPool();
  Value->InitValueEntry(Val);
  return Value;
}

std::unique_ptr<FrameEntry> MemoryPool::requestFrameEntryFromPool() {
  if (FrameEntryPool.size() == 0) {
    for (unsigned int I = 0; I < FrameEntryCnt; I++) {
      FrameEntryPool.push_back(std::make_unique<FrameEntry>());
    }
    FrameEntryCnt *= 2;
  }
  std::unique_ptr<FrameEntry> Frame = std::move(FrameEntryPool.back());
  FrameEntryPool.pop_back();
  return Frame;
}

std::unique_ptr<LabelEntry> MemoryPool::requestLabelEntryFromPool() {
  if (LabelEntryPool.size() == 0) {
    for (unsigned int I = 0; I < LabelEntryCnt; I++) {
      LabelEntryPool.push_back(std::make_unique<LabelEntry>());
    }
    LabelEntryCnt *= 2;
  }
  std::unique_ptr<LabelEntry> Label = std::move(LabelEntryPool.back());
  LabelEntryPool.pop_back();
  return Label;
}

std::unique_ptr<ValueEntry> MemoryPool::requestValueEntryFromPool() {
  if (ValueEntryPool.size() == 0) {
    for (unsigned int I = 0; I < ValueEntryCnt; I++) {
      ValueEntryPool.push_back(std::make_unique<ValueEntry>());
    }
    ValueEntryCnt *= 2;
  }
  std::unique_ptr<ValueEntry> Value = std::move(ValueEntryPool.back());
  ValueEntryPool.pop_back();
  return Value;
}

} // namespace Executor
} // namespace SSVM
