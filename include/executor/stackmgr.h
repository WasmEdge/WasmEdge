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
  Executor::ErrCode getTop(FrameEntry *&Frame);
  Executor::ErrCode getTop(LabelEntry *&Label);
  Executor::ErrCode getTop(ValueEntry *&Value);

  /// Push a new entry to stack.
  Executor::ErrCode push(EntryType NewEntry);

  /// Pop and return the top entry.
  Executor::ErrCode pop(std::unique_ptr<FrameEntry> &Frame);
  Executor::ErrCode pop(std::unique_ptr<LabelEntry> &Label);
  Executor::ErrCode pop(std::unique_ptr<ValueEntry> &Value);

  /// Drop the top entry.
  Executor::ErrCode drop();

  /// Frame related operation.
  Executor::ErrCode getCurrentFrame(FrameEntry *&Frame);

  /// Label related operation.
  Executor::ErrCode getLabelWithCount(LabelEntry *&Label);

  /// Checking the top entry's attribute
  bool isTopFrame();
  bool isTopLabel();
  bool isTopValue();

private:
  std::vector<EntryType> Stack;
  std::vector<unsigned int> LabelIdx;
};