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
#include "memorypool.h"
#include "support/casting.h"

#include <memory>
#include <type_traits>
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
template <typename T>
inline constexpr const bool IsEntryV =
    std::is_same_v<T, FrameEntry> || std::is_same_v<T, LabelEntry> ||
    std::is_same_v<T, ValueEntry>;

/// Accept entry types.
template <typename T, typename TR>
using TypeE = typename std::enable_if_t<IsEntryV<T>, TR>;

/// Accept Wasm built-in types. (uint32_t, uint64_t, float, double)
template <typename T, typename TR>
using TypeB = typename std::enable_if_t<Support::IsWasmBuiltInV<T>, TR>;
} // namespace

class StackManager {
public:
  StackManager() = delete;
  explicit StackManager(MemoryPool &Pool) : MemPool(Pool){};
  ~StackManager() = default;

  /// Getters of top entry of stack.
  template <typename T> TypeE<T, ErrCode> getTop(T *&Entry);

  /// Push a new entry to stack.
  template <typename T> TypeE<T, ErrCode> push(std::unique_ptr<T> &&NewEntry);
  template <typename T> TypeE<T, ErrCode> push(std::unique_ptr<T> &NewEntry) {
    return push(std::move(NewEntry));
  }
  template <typename T> TypeB<T, ErrCode> pushValue(T Val) {
    Stack.push_back(std::move(MemPool.getValueEntry(Val)));
    return ErrCode::Success;
  }

  /// Pop and return the top entry.
  template <typename T> TypeE<T, ErrCode> pop(std::unique_ptr<T> &Entry);

  /// Pop the top entry.
  inline ErrCode pop() {
    /// Check the size of stack.
    if (Stack.size() == 0)
      return ErrCode::StackEmpty;

    /// Check is the top entry's type.
    switch (Stack.back().index()) {
    case 0: /// Frame entry
      FrameIdx.pop_back();
      MemPool.recycleFrameEntry(std::move(std::get<0>(Stack.back())));
      break;
    case 1: /// Label entry
      LabelIdx.pop_back();
      MemPool.recycleLabelEntry(std::move(std::get<1>(Stack.back())));
      break;
    case 2: /// Value entry
      MemPool.recycleValueEntry(std::move(std::get<2>(Stack.back())));
      break;
    default:
      break;
    }

    /// Drop the top entry.
    Stack.pop_back();
    return ErrCode::Success;
  }

  /// Get the current toppest frame.
  inline ErrCode getCurrentFrame(FrameEntry *&Frame) {
    /// Check is there current frame.
    if (FrameIdx.size() == 0)
      return ErrCode::WrongInstanceAddress;

    /// Get the current frame pointer.
    Frame = std::get<0>(Stack[FrameIdx.back()]).get();
    return ErrCode::Success;
  }

  /// Get the top of count of label.
  inline ErrCode getLabelWithCount(LabelEntry *&Label, unsigned int Count) {
    /// Check is there at least count + 1 labels.
    if (LabelIdx.size() < Count + 1)
      return ErrCode::WrongInstanceAddress;

    /// Get the (count + 1)-th top of label.
    unsigned int Idx = LabelIdx.size() - Count - 1;
    Label = std::get<1>(Stack[LabelIdx[Idx]]).get();
    return ErrCode::Success;
  }

  /// Checking the top entry's attribute
  bool isTopFrame() { return (Stack.size() > 0) && Stack.back().index() == 0; }
  bool isTopLabel() { return (Stack.size() > 0) && Stack.back().index() == 1; }
  bool isTopValue() { return (Stack.size() > 0) && Stack.back().index() == 2; }

  /// Reset stack.
  ErrCode reset() {
    /// Recycle entries in stack.
    while (Stack.size() > 0) {
      switch (Stack.back().index()) {
      case 0: /// Frame entry
        MemPool.recycleFrameEntry(std::move(std::get<0>(Stack.back())));
        break;
      case 1: /// Label entry
        MemPool.recycleLabelEntry(std::move(std::get<1>(Stack.back())));
        break;
      default:
        break;
      }
      Stack.pop_back();
    }
    LabelIdx.clear();
    FrameIdx.clear();
    return ErrCode::Success;
  }

private:
  /// \name Data of stack manager.
  /// @{
  MemoryPool &MemPool;
  std::vector<EntryType> Stack;
  std::vector<unsigned int> LabelIdx;
  std::vector<unsigned int> FrameIdx;
  /// @}
};

} // namespace Executor
} // namespace SSVM

#include "stackmgr.ipp"