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

  /// Get and initialize label entry.
  std::unique_ptr<LabelEntry> getLabelEntry(const unsigned int LabelArity,
                                            AST::Instruction *Instr = nullptr);

  /// Get and initialize value entry.
  std::unique_ptr<ValueEntry> getValueEntry() { return getValueEntry(0U); }
  std::unique_ptr<ValueEntry> getValueEntry(const ValueEntry &VE);
  std::unique_ptr<ValueEntry> getValueEntry(const AST::ValType &VT);
  std::unique_ptr<ValueEntry> getValueEntry(const AST::ValType &VT,
                                            const AST::ValVariant &Val);
  std::unique_ptr<ValueEntry> getValueEntry(const AST::ValVariant &Val);
  template <typename T>
  inline std::enable_if_t<Support::IsWasmBuiltInV<T>,
                          std::unique_ptr<ValueEntry>>
  getValueEntry(const T &Val) {
    std::unique_ptr<ValueEntry> Value = requestValueEntryFromPool();
    Value->InitValueEntry(Val);
    return Value;
  }

  /// Recycle frame entry.
  void recycleFrameEntry(std::unique_ptr<FrameEntry> Frame) {
    FrameEntryPool.push_back(std::move(Frame));
  }

  /// Recycle frame entry.
  void recycleLabelEntry(std::unique_ptr<LabelEntry> Label) {
    LabelEntryPool.push_back(std::move(Label));
  }

  /// Recycle frame entry.
  void recycleValueEntry(std::unique_ptr<ValueEntry> Value) {
    ValueEntryPool.push_back(std::move(Value));
  }

private:
  unsigned int FrameEntryCnt;
  unsigned int LabelEntryCnt;
  unsigned int ValueEntryCnt;
  std::vector<std::unique_ptr<FrameEntry>> FrameEntryPool;
  std::vector<std::unique_ptr<LabelEntry>> LabelEntryPool;
  std::vector<std::unique_ptr<ValueEntry>> ValueEntryPool;
  std::unique_ptr<FrameEntry> requestFrameEntryFromPool();
  std::unique_ptr<LabelEntry> requestLabelEntryFromPool();
  std::unique_ptr<ValueEntry> requestValueEntryFromPool();
};

} // namespace Executor
} // namespace SSVM