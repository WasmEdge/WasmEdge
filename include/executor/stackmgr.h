//===-- ssvm/executor/stackmgr.h - Stack Manager definition ---------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Stack Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "entry/frame.h"
#include "entry/label.h"
#include "entry/value.h"
#include <memory>
#include <variant>
#include <vector>

namespace SSVM {
namespace Executor {

class StackManager {
public:
  StackManager() = default;
  ~StackManager() = default;
  /// Variant of entry classes.
  using EntryType =
      std::variant<std::unique_ptr<FrameEntry>, std::unique_ptr<LabelEntry>,
                   std::unique_ptr<ValueEntry>>;

  /// Getters of top entry of stack.
  template <typename T> ErrCode getTop(T *&Entry);

  /// Push a new entry to stack.
  template <typename T> ErrCode push(std::unique_ptr<T> &NewEntry);

  /// Pop and return the top entry.
  template <typename T> ErrCode pop(std::unique_ptr<T> &Entry);

  /// Pop the top entry.
  ErrCode pop();

  /// Get the current toppest frame.
  ErrCode getCurrentFrame(FrameEntry *&Frame);

  /// Get the top of count of label.
  ErrCode getLabelWithCount(LabelEntry *&Label, unsigned int Count);

  /// Checking the top entry's attribute
  bool isTopFrame() { return (Stack.size() > 0) && Stack.back().index() == 0; }
  bool isTopLabel() { return (Stack.size() > 0) && Stack.back().index() == 1; }
  bool isTopValue() { return (Stack.size() > 0) && Stack.back().index() == 2; }

private:
  /// \name Data of value entry.
  /// @{
  std::vector<EntryType> Stack;
  std::vector<unsigned int> LabelIdx;
  std::vector<unsigned int> FrameIdx;
  /// @}
};

} // namespace Executor
} // namespace SSVM
