// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/stackmgr.h - Stack Manager definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Stack Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

#include <optional>
#include <vector>

namespace WasmEdge {
namespace Runtime {

class StackManager {
public:
  struct Frame {
    Frame() = delete;
    Frame(uint32_t Addr, uint32_t L, uint32_t A,
          AST::InstrView::iterator FromIt, const bool Dummy = false) noexcept
        : ModAddr(Addr), Locals(L), Arity(A), From(FromIt), IsDummy(Dummy) {}
    uint32_t ModAddr;
    uint32_t Locals;
    uint32_t Arity;
    AST::InstrView::iterator From;
    bool IsDummy;
  };

  using Value = ValVariant;

  /// Stack manager provides the stack control for Wasm execution with VALIDATED
  /// modules. All operations of instructions passed validation, therefore no
  /// unexpect operations will occur.
  StackManager() noexcept {
    ValueStack.reserve(2048U);
    FrameStack.reserve(16U);
  }
  ~StackManager() = default;

  /// Getter of stack size.
  size_t size() const noexcept { return ValueStack.size(); }

  /// Push a new value entry to stack.
  template <typename T,
            typename = std::enable_if_t<
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                ValVariant> &&
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                RefVariant>>>
  void push(T Val) noexcept {
    ValueStack.push_back(Val);
  }

  /// Unsafe Pop and return the top entry.
  template <typename T,
            typename = std::enable_if_t<
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                ValVariant> &&
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                RefVariant>>>
  T pop() noexcept {
    T V = std::move(ValueStack.back()).get<T>();
    ValueStack.pop_back();
    return V;
  }

  void push(ValType Type, ValVariant Val) noexcept {
    switch (Type) {
    case ValType::I32:
      return push<uint32_t>(Val.get<uint32_t>());
    case ValType::F32:
      return push<float>(Val.get<float>());
    case ValType::I64:
      return push<uint64_t>(Val.get<uint64_t>());
    case ValType::F64:
      return push<double>(Val.get<double>());
    case ValType::FuncRef:
    case ValType::ExternRef:
      return push<UnknownRef>(Val.get<UnknownRef>());
    case ValType::V128:
      return push<uint128_t>(Val.get<uint128_t>());
    case ValType::None:
    default:
      assumingUnreachable();
    }
  }

  ValVariant pop(ValType Type) noexcept {
    switch (Type) {
    case ValType::I32:
      return pop<uint32_t>();
    case ValType::F32:
      return pop<float>();
    case ValType::I64:
      return pop<uint64_t>();
    case ValType::F64:
      return pop<double>();
    case ValType::FuncRef:
    case ValType::ExternRef:
      return pop<UnknownRef>();
    case ValType::V128:
      return pop<uint128_t>();
    case ValType::None:
    default:
      assumingUnreachable();
    }
  }

  template <typename T,
            typename = std::enable_if_t<
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                ValVariant> &&
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                RefVariant>>>
  T getTopN(uint32_t Offset) const noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    T V = ValueStack[ValueStack.size() - Offset].get<T>();
    return V;
  }

  ValVariant getTopN(uint32_t Offset, ValType Type) const noexcept {
    switch (Type) {
    case ValType::I32:
      return getTopN<uint32_t>(Offset);
    case ValType::F32:
      return getTopN<float>(Offset);
    case ValType::I64:
      return getTopN<uint64_t>(Offset);
    case ValType::F64:
      return getTopN<double>(Offset);
    case ValType::FuncRef:
    case ValType::ExternRef:
      return getTopN<UnknownRef>(Offset);
    case ValType::V128:
      return getTopN<uint128_t>(Offset);
    case ValType::None:
    default:
      assumingUnreachable();
    }
  }

  template <typename T,
            typename = std::enable_if_t<
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                ValVariant> &&
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                RefVariant>>>
  void setTopN(uint32_t Offset, T Val) noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    ValueStack[ValueStack.size() - Offset].get<T>() = Val;
  }

  void setTopN(uint32_t Offset, ValType Type, ValVariant Val) noexcept {
    switch (Type) {
    case ValType::I32:
      return setTopN<uint32_t>(Offset, Val.get<uint32_t>());
    case ValType::F32:
      return setTopN<float>(Offset, Val.get<float>());
    case ValType::I64:
      return setTopN<uint64_t>(Offset, Val.get<uint64_t>());
    case ValType::F64:
      return setTopN<double>(Offset, Val.get<double>());
    case ValType::FuncRef:
    case ValType::ExternRef:
      return setTopN<UnknownRef>(Offset, Val.get<UnknownRef>());
    case ValType::V128:
      return setTopN<uint128_t>(Offset, Val.get<uint128_t>());
    case ValType::None:
    default:
      assumingUnreachable();
    }
  }

  /// Push a new frame entry to stack.
  void pushFrame(const uint32_t ModuleAddr, AST::InstrView::iterator From,
                 const uint32_t LocalNum = 0,
                 const uint32_t Arity = 0) noexcept {
    FrameStack.emplace_back(ModuleAddr, LocalNum, Arity, From);
  }

  /// Push a dummy frame for invokation base.
  void pushDummyFrame() noexcept {
    FrameStack.emplace_back(0, ValueStack.size(), 0, AST::InstrView::iterator{},
                            true);
  }

  /// Unsafe pop top frame.
  AST::InstrView::iterator popFrame() noexcept {
    assuming(!FrameStack.empty());
    assuming(ValueStack.size() >=
             FrameStack.back().Locals + FrameStack.back().Arity);
    ValueStack.erase(ValueStack.end() - FrameStack.back().Locals -
                         FrameStack.back().Arity,
                     ValueStack.end() - FrameStack.back().Arity);
    auto From = FrameStack.back().From;
    FrameStack.pop_back();
    return From;
  }

  /// Unsafe erase stack.
  void stackErase(uint32_t EraseBegin, uint32_t EraseEnd) noexcept {
    assuming(EraseEnd <= EraseBegin && EraseBegin <= ValueStack.size());
    ValueStack.erase(ValueStack.end() - EraseBegin,
                     ValueStack.end() - EraseEnd);
  }

  /// Unsafe leave top label.
  AST::InstrView::iterator maybePopFrame(AST::InstrView::iterator PC) noexcept {
    if (FrameStack.size() > 1 && PC->isLast()) {
      // Noted that there's always a base frame in stack.
      return popFrame();
    }
    return PC;
  }

  /// Unsafe getter of module address.
  uint32_t getModuleAddr() const noexcept {
    assuming(!FrameStack.empty());
    return FrameStack.back().ModAddr;
  }

  /// Unsafe checker of top frame is a dummy frame.
  bool isTopDummyFrame() noexcept {
    assuming(!FrameStack.empty());
    return FrameStack.back().IsDummy;
  }

  /// Reset stack.
  void reset() noexcept {
    ValueStack.clear();
    FrameStack.clear();
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> ValueStack;
  std::vector<Frame> FrameStack;
  /// @}
};

} // namespace Runtime
} // namespace WasmEdge
