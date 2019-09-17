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
#include "support/casting.h"

#include <memory>
#include <variant>
#include <vector>

namespace SSVM {
namespace Executor {

namespace {
/// Variant of entry classes.
using EntryType =
    std::variant<std::unique_ptr<FrameEntry>, std::unique_ptr<LabelEntry>,
                 std::unique_ptr<ValueEntry>>;

/// Return true if T is entry types.
template <typename T> struct IsEntry {
  static constexpr const bool value =
      (std::is_same_v<T, FrameEntry> || std::is_same_v<T, LabelEntry> ||
       std::is_same_v<T, ValueEntry>);
};

/// Accept entry types.
template <typename T, typename TR>
using TypeE = typename std::enable_if_t<IsEntry<T>::value, TR>;

/// Accept Wasm built-in types. (uint32_t, uint64_t, float, double)
template <typename T, typename TR>
using TypeB = typename std::enable_if_t<Support::IsWasmBuiltIn<T>::value, TR>;
} // namespace

class StackManager {
public:
  StackManager() = default;
  ~StackManager() = default;

  /// Getters of top entry of stack.
  template <typename T> TypeE<T, ErrCode> getTop(T *&Entry);

  /// Push a new entry to stack.
  template <typename T> TypeE<T, ErrCode> push(std::unique_ptr<T> &&NewEntry);
  template <typename T> TypeE<T, ErrCode> push(std::unique_ptr<T> &NewEntry) {
    return push(std::move(NewEntry));
  }
  template <typename T> TypeB<T, ErrCode> pushValue(T Val);

  /// Pop and return the top entry.
  template <typename T> TypeE<T, ErrCode> pop(std::unique_ptr<T> &Entry);

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
