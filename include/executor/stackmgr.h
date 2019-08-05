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
#include "frameentry.h"
#include "labelentry.h"
#include "valueentry.h"
#include <memory>
#include <vector>

class StackMgr {
public:
  /// Variant of entry classes.
  using EntryType =
      std::variant<std::unique_ptr<FrameEntry>, std::unique_ptr<LabelEntry>,
                   std::unique_ptr<ValueEntry>>;

  /// Getters of top entry of stack.
  template <typename T> Executor::ErrCode getTop(T *&Entry);

  /// Push a new entry to stack.
  template <typename T> Executor::ErrCode push(std::unique_ptr<T> &NewEntry);

  /// Pop and return the top entry.
  template <typename T> Executor::ErrCode pop(std::unique_ptr<T> &Entry);

  /// Drop the top entry.
  Executor::ErrCode drop();

  /// Get the current toppest frame.
  Executor::ErrCode getCurrentFrame(FrameEntry *&Frame);

  /// Get the top of count of label.
  Executor::ErrCode getLabelWithCount(LabelEntry *&Label, unsigned int Count);

  /// Checking the top entry's attribute
  bool isTopFrame();
  bool isTopLabel();
  bool isTopValue();

private:
  /// \name Data of value entry.
  /// @{
  std::vector<EntryType> Stack;
  std::vector<unsigned int> LabelIdx;
  std::vector<unsigned int> FrameIdx;
  /// @}
};