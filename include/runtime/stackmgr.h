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

#include <optional>
#include <vector>

namespace WasmEdge {
namespace Runtime {

class StackManager {
public:
  struct Label {
    Label() = delete;
    Label(const uint32_t S, const uint32_t A, AST::InstrView::iterator FromIt,
          std::optional<AST::InstrView::iterator> ContIt)
        : VStackOff(S), Arity(A), From(FromIt), Cont(ContIt) {}
    uint32_t VStackOff;
    uint32_t Arity;
    AST::InstrView::iterator From;
    std::optional<AST::InstrView::iterator> Cont;
  };

  struct Frame {
    Frame() = delete;
    Frame(const uint32_t Addr, const uint32_t VS, const uint32_t LS,
          const uint32_t A, const bool Dummy = false)
        : ModAddr(Addr), VStackOff(VS), LStackOff(LS), Arity(A),
          IsDummy(Dummy) {}
    uint32_t ModAddr;
    uint32_t VStackOff;
    uint32_t LStackOff;
    uint32_t Arity;
    bool IsDummy;
  };

  using Value = ValVariant;

  /// Stack manager provides the stack control for Wasm execution with VALIDATED
  /// modules. All operations of instructions passed validation, therefore no
  /// unexpect operations will occur.
  StackManager() {
    ValueStack.reserve(2048U);
    LabelStack.reserve(64U);
    FrameStack.reserve(16U);
  }
  ~StackManager() = default;

  /// Getter of stack size.
  size_t size() const { return ValueStack.size(); }

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
  void pushFrame(const uint32_t ModuleAddr, const uint32_t LocalNum = 0,
                 const uint32_t ArityNum = 0) {
    FrameStack.emplace_back(ModuleAddr, ValueStack.size() - LocalNum,
                            LabelStack.size(), ArityNum);
  }

  /// Push a dummy frame for invokation base.
  void pushDummyFrame() {
    FrameStack.emplace_back(0, ValueStack.size(), LabelStack.size(), 0, true);
  }

  /// Unsafe pop top frame.
  void popFrame() {
    assuming(LabelStack.size() >= FrameStack.back().LStackOff);
    LabelStack.erase(LabelStack.begin() + FrameStack.back().LStackOff,
                     LabelStack.end());
    assuming(ValueStack.size() >=
             FrameStack.back().VStackOff + FrameStack.back().Arity);
    ValueStack.erase(ValueStack.begin() + FrameStack.back().VStackOff,
                     ValueStack.end() - FrameStack.back().Arity);
    FrameStack.pop_back();
  }

  /// Push a new label entry to stack.
  void pushLabel(const uint32_t LocalNum, const uint32_t ArityNum,
                 AST::InstrView::iterator From,
                 std::optional<AST::InstrView::iterator> Cont = std::nullopt) {
    LabelStack.emplace_back(ValueStack.size() - LocalNum, ArityNum, From, Cont);
  }

  /// Unsafe pop top label.
  AST::InstrView::iterator popLabel(uint32_t Cnt, uint32_t EraseBegin,
                                    uint32_t EraseEnd) noexcept {
    const auto &L = getLabelWithCount(Cnt - 1);
    assuming(EraseBegin == ValueStack.size() - L.VStackOff);
    assuming(EraseEnd == L.Arity);
    ValueStack.erase(ValueStack.end() - EraseBegin,
                     ValueStack.end() - EraseEnd);
    auto It = L.From;
    LabelStack.erase(LabelStack.end() - Cnt, LabelStack.end());
    return It;
  }

  /// Unsafe leave top label.
  AST::InstrView::iterator leaveLabel() {
    auto It = LabelStack.back().From;
    LabelStack.pop_back();
    if (FrameStack.size() > 1 &&
        FrameStack.back().LStackOff == LabelStack.size()) {
      // Noted that there's always a base frame in stack.
      popFrame();
    }
    return It;
  }

  /// Unsafe getter of module address.
  uint32_t getModuleAddr() const { return FrameStack.back().ModAddr; }

  /// Unsafe getter for stack offset of local values by index.
  uint32_t getOffset(uint32_t Idx) const {
    return FrameStack.back().VStackOff + Idx;
  }

  /// Unsafe getter of the top count of label which index start from 0.
  const Label &getLabelWithCount(const uint32_t Count) const {
    return LabelStack[LabelStack.size() - Count - 1];
  }

  /// Unsafe getter of the bottom label on the top frame.
  const Label &getBottomLabel() const {
    return LabelStack[FrameStack.back().LStackOff];
  }

  /// Unsafe checker of top frame is a dummy frame.
  bool isTopDummyFrame() { return FrameStack.back().IsDummy; }

  /// Reset stack.
  void reset() {
    ValueStack.clear();
    LabelStack.clear();
    FrameStack.clear();
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> ValueStack;
  std::vector<Label> LabelStack;
  std::vector<Frame> FrameStack;
  /// @}
};

} // namespace Runtime
} // namespace WasmEdge
