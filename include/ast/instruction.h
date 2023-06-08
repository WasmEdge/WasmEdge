// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/ast/instruction.h - Instruction class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instruction node class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_ast.hpp"
#include "common/span.h"
#include "common/types.h"

#include <algorithm>
#include <vector>

namespace WasmEdge {
namespace AST {

/// Instruction node class.
class Instruction {
public:
  struct JumpDescriptor {
    uint32_t TargetIndex;
    uint32_t ValueStackEraseBegin;
    uint32_t ValueStackEraseEnd;
    uint32_t HandlerStackOffset;
    uint32_t CaughtStackOffset;
    int32_t PCOffset;
  };

public:
  /// Constructor assigns the OpCode and the Offset.
  Instruction(OpCode Byte, uint32_t Off = 0) noexcept
      : Offset(Off), Code(Byte) {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    Data.Num = static_cast<uint128_t>(0);
#else
    Data.Num.Low = static_cast<uint64_t>(0);
    Data.Num.High = static_cast<uint64_t>(0);
#endif
    // Initializing JumpCatchAll to check whether it is set during loading phase
    Data.TryBlock.JumpCatchAll = 0;

    Flags.IsAllocLabelList = false;
    Flags.IsAllocValTypeList = false;
    Flags.IsAllocJumpCatchList = false;
    Flags.IsTryLast = false;
    Flags.IsCatchLast = false;
    Flags.IsDelegate = false;
  }

  /// Copy constructor.
  Instruction(const Instruction &Instr)
      : Data(Instr.Data), Offset(Instr.Offset), Code(Instr.Code),
        Flags(Instr.Flags) {
    if (Flags.IsAllocLabelList) {
      Data.BrTable.LabelList = new JumpDescriptor[Data.BrTable.LabelListSize];
      std::copy_n(Instr.Data.BrTable.LabelList, Data.BrTable.LabelListSize,
                  Data.BrTable.LabelList);
    } else if (Flags.IsAllocValTypeList) {
      Data.SelectT.ValTypeList = new ValType[Data.SelectT.ValTypeListSize];
      std::copy_n(Instr.Data.SelectT.ValTypeList, Data.SelectT.ValTypeListSize,
                  Data.SelectT.ValTypeList);
    } else if (Flags.IsAllocJumpCatchList) {
      Data.TryBlock.JumpCatchList =
          new uint32_t[Data.TryBlock.JumpCatchListSize];
      std::copy_n(Instr.Data.TryBlock.JumpCatchList,
                  Data.TryBlock.JumpCatchListSize, Data.TryBlock.JumpCatchList);
    }
  }

  /// Move constructor.
  Instruction(Instruction &&Instr)
      : Data(Instr.Data), Offset(Instr.Offset), Code(Instr.Code),
        Flags(Instr.Flags) {
    Instr.Flags.IsAllocLabelList = false;
    Instr.Flags.IsAllocValTypeList = false;
    Instr.Flags.IsAllocJumpCatchList = false;
  }

  /// Destructor.
  ~Instruction() { reset(); }

  /// Copy assignment.
  Instruction &operator=(const Instruction &Instr) {
    if (this != &Instr) {
      Instruction Tmp(Instr);
      Tmp.swap(*this);
    }
    return *this;
  }

  /// Getter of OpCode.
  OpCode getOpCode() const noexcept { return Code; }

  /// Getter of Offset.
  uint32_t getOffset() const noexcept { return Offset; }

  /// Getter and setter of block type.
  BlockType getBlockType() const noexcept {
    return Code == OpCode::Try ? Data.TryBlock.ResType : Data.Blocks.ResType;
  }
  void setBlockType(ValType VType) noexcept {
    Code == OpCode::Try ? Data.TryBlock.ResType.setData(VType)
                        : Data.Blocks.ResType.setData(VType);
  }
  void setBlockType(uint32_t Idx) noexcept {
    Code == OpCode::Try ? Data.TryBlock.ResType.setData(Idx)
                        : Data.Blocks.ResType.setData(Idx);
  }
  void setEmptyBlockType() noexcept {
    Code == OpCode::Try ? Data.TryBlock.ResType.setEmpty()
                        : Data.Blocks.ResType.setEmpty();
  }

  /// Getter and setter of jump count to End instruction.
  uint32_t getJumpEnd() const noexcept { return Data.Blocks.JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) noexcept { Data.Blocks.JumpEnd = Cnt; }

  /// Getter and setter of jump count to Else instruction.
  uint32_t getJumpElse() const noexcept { return Data.Blocks.JumpElse; }
  void setJumpElse(const uint32_t Cnt) noexcept { Data.Blocks.JumpElse = Cnt; }

  /// Getter and setter of reference type.
  RefType getRefType() const noexcept { return Data.ReferenceType; }
  void setRefType(RefType RType) noexcept { Data.ReferenceType = RType; }

  /// Getter and setter of label list.
  void setLabelListSize(uint32_t Size) {
    reset();
    if (Size > 0) {
      Data.BrTable.LabelListSize = Size;
      Data.BrTable.LabelList = new JumpDescriptor[Size];
      Flags.IsAllocLabelList = true;
    }
  }
  Span<const JumpDescriptor> getLabelList() const noexcept {
    return Span<const JumpDescriptor>(
        Data.BrTable.LabelList,
        Flags.IsAllocLabelList ? Data.BrTable.LabelListSize : 0);
  }
  Span<JumpDescriptor> getLabelList() noexcept {
    return Span<JumpDescriptor>(
        Data.BrTable.LabelList,
        Flags.IsAllocLabelList ? Data.BrTable.LabelListSize : 0);
  }

  /// Getter and setter of IsLast for End instruction.
  bool isLast() const noexcept { return Data.IsLast; }
  void setLast(bool Last = true) noexcept { Data.IsLast = Last; }

  /// Getter and setter of Jump for Br* instruction.
  const JumpDescriptor &getJump() const noexcept { return Data.Jump; }
  JumpDescriptor &getJump() noexcept { return Data.Jump; }

  /// Getter and setter of selecting value types list.
  void setValTypeListSize(uint32_t Size) {
    reset();
    if (Size > 0) {
      Data.SelectT.ValTypeListSize = Size;
      Data.SelectT.ValTypeList = new ValType[Size];
      Flags.IsAllocValTypeList = true;
    }
  }
  Span<const ValType> getValTypeList() const noexcept {
    return Span<const ValType>(Data.SelectT.ValTypeList,
                               Data.SelectT.ValTypeListSize);
  }
  Span<ValType> getValTypeList() noexcept {
    return Span<ValType>(Data.SelectT.ValTypeList,
                         Data.SelectT.ValTypeListSize);
  }

  /// Getter and setter of target index.
  uint32_t getTargetIndex() const noexcept { return Data.Indices.TargetIdx; }
  uint32_t &getTargetIndex() noexcept { return Data.Indices.TargetIdx; }

  /// Getter and setter of source index.
  uint32_t getSourceIndex() const noexcept { return Data.Indices.SourceIdx; }
  uint32_t &getSourceIndex() noexcept { return Data.Indices.SourceIdx; }

  /// Getter and setter of stack offset.
  uint32_t getStackOffset() const noexcept { return Data.Indices.StackOffset; }
  uint32_t &getStackOffset() noexcept { return Data.Indices.StackOffset; }

  /// Getter and setter of memory alignment.
  uint32_t getMemoryAlign() const noexcept { return Data.Memories.MemAlign; }
  uint32_t &getMemoryAlign() noexcept { return Data.Memories.MemAlign; }

  /// Getter of memory offset.
  uint32_t getMemoryOffset() const noexcept { return Data.Memories.MemOffset; }
  uint32_t &getMemoryOffset() noexcept { return Data.Memories.MemOffset; }

  /// Getter of memory lane.
  uint8_t getMemoryLane() const noexcept { return Data.Memories.MemLane; }
  uint8_t &getMemoryLane() noexcept { return Data.Memories.MemLane; }

  /// Getter and setter of jump catch list.
  void setJumpCatchList(const std::vector<uint32_t> &CatchList) {
    reset();
    if (!CatchList.empty()) {
      Data.TryBlock.JumpCatchListSize = CatchList.size();
      Data.TryBlock.JumpCatchList = new uint32_t[CatchList.size()];
      Flags.IsAllocJumpCatchList = true;
      std::copy_n(CatchList.begin(), CatchList.size(),
                  Data.TryBlock.JumpCatchList);
    }
  }
  Span<const uint32_t> getJumpCatchList() const noexcept {
    return Span<const uint32_t>(
        Data.TryBlock.JumpCatchList,
        Flags.IsAllocJumpCatchList ? Data.TryBlock.JumpCatchListSize : 0);
  }
  Span<uint32_t> getJumpCatchList() noexcept {
    return Span<uint32_t>(
        Data.TryBlock.JumpCatchList,
        Flags.IsAllocJumpCatchList ? Data.TryBlock.JumpCatchListSize : 0);
  }

  /// Getter and setter of jump count to catch_all instruction.
  uint32_t getJumpCatchAll() const noexcept {
    return Data.TryBlock.JumpCatchAll;
  }
  void setJumpCatchAll(const uint32_t Cnt) noexcept {
    Data.TryBlock.JumpCatchAll = Cnt;
  }

  /// Getter and setter of jump count to the end of try block.
  uint32_t getTryBlockJumpEnd() const noexcept { return Data.TryBlock.JumpEnd; }
  void setTryBlockJumpEnd(const uint32_t Cnt) noexcept {
    Data.TryBlock.JumpEnd = Cnt;
  }

  /// Getter and setter of number of block type parameter.
  uint32_t getTryBlockParamNum() const noexcept {
    return Data.TryBlock.BlockParamNum;
  }
  void setTryBlockParamNum(const uint32_t Num) noexcept {
    Data.TryBlock.BlockParamNum = Num;
  }

  /// Getter and setter of jump count to delegate instruction.
  uint32_t getDelegateIdx() const noexcept { return Data.TryBlock.DelegateIdx; }
  void setDelegateIdx(const uint32_t Cnt) noexcept {
    Data.TryBlock.DelegateIdx = Cnt;
  }

  /// Getter and setter of VSize
  uint32_t getTryBlockVSize() const noexcept { return Data.TryBlock.VSize; }
  void setTryBlockVSize(const uint32_t Num) noexcept {
    Data.TryBlock.VSize = Num;
  }

  /// Getter and setter of HOffset
  uint32_t getTryBlockHOffset() const noexcept { return Data.TryBlock.HOffset; }
  void setTryBlockHOffset(const uint32_t Num) noexcept {
    Data.TryBlock.HOffset = Num;
  }

  /// Getter and setter of COffset
  uint32_t getTryBlockCOffset() const noexcept { return Data.TryBlock.COffset; }
  void setTryBlockCOffset(const uint32_t Num) noexcept {
    Data.TryBlock.COffset = Num;
  }

  /// Getter and setter of the constant value.
  ValVariant getNum() const noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    return ValVariant(Data.Num);
#else
    uint128_t N(Data.Num.High, Data.Num.Low);
    return ValVariant(N);
#endif
  }
  void setNum(ValVariant N) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    Data.Num = N.get<uint128_t>();
#else
    std::memcpy(&Data.Num, &N.get<uint128_t>(), sizeof(uint128_t));
#endif
  }

  /// Getter and setter of IsTryLast for End instruction.
  bool isTryLast() const noexcept { return Flags.IsTryLast; }
  void setTryLast(bool Last = true) noexcept { Flags.IsTryLast = Last; }

  /// Getter and setter of IsCatchLast for End instruction.
  bool isCatchLast() const noexcept { return Flags.IsCatchLast; }
  void setCatchLast(bool Last = true) noexcept { Flags.IsCatchLast = Last; }

  /// Getter and setter of IsDelegate for Try instruction.
  bool isDelegate() const noexcept { return Flags.IsDelegate; }
  void setDelegate(bool Last = true) noexcept { Flags.IsDelegate = Last; }

private:
  /// Release allocated resources.
  void reset() {
    if (Flags.IsAllocLabelList) {
      Data.BrTable.LabelListSize = 0;
      delete[] Data.BrTable.LabelList;
    } else if (Flags.IsAllocValTypeList) {
      Data.SelectT.ValTypeListSize = 0;
      delete[] Data.SelectT.ValTypeList;
    } else if (Flags.IsAllocJumpCatchList) {
      Data.TryBlock.JumpCatchListSize = 0;
      delete[] Data.TryBlock.JumpCatchList;
    }
    Flags.IsAllocLabelList = false;
    Flags.IsAllocValTypeList = false;
    Flags.IsAllocJumpCatchList = false;
  }

  /// Swap function.
  void swap(Instruction &Instr) noexcept {
    std::swap(Data, Instr.Data);
    std::swap(Offset, Instr.Offset);
    std::swap(Code, Instr.Code);
    std::swap(Flags, Instr.Flags);
  }

  /// \name Data of instructions.
  /// @{
  union Inner {
    // Type 1: BlockType, JumpEnd, and JumpElse.
    struct {
      uint32_t JumpEnd;
      uint32_t JumpElse;
      BlockType ResType;
    } Blocks;
    // Type 2: TargetIdx, SourceIdx and StackOffset.
    struct {
      uint32_t TargetIdx;
      uint32_t SourceIdx;
      uint32_t StackOffset;
    } Indices;
    // Type 3: Jump.
    JumpDescriptor Jump;
    // Type 4: LabelList.
    struct {
      uint32_t LabelListSize;
      JumpDescriptor *LabelList;
    } BrTable;
    // Type 5: RefType.
    RefType ReferenceType;
    // Type 6: ValTypeList.
    struct {
      uint32_t ValTypeListSize;
      ValType *ValTypeList;
    } SelectT;
    // Type 7: TargetIdx, MemAlign, MemOffset, and MemLane.
    struct {
      uint32_t TargetIdx;
      uint32_t MemAlign;
      uint32_t MemOffset;
      uint8_t MemLane;
    } Memories;
    // Type 8: Num.
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    uint128_t Num;
#else
    struct {
      uint64_t Low;
      uint64_t High;
    } Num;
#endif
    // Type 9: IsLast.
    bool IsLast;
    // Type 10: Try Block
    struct {
      uint32_t JumpCatchListSize;
      uint32_t *JumpCatchList;
      uint32_t JumpCatchAll;
      uint32_t JumpEnd;
      uint32_t BlockParamNum;
      uint32_t DelegateIdx;
      uint32_t VSize;
      uint32_t HOffset;
      uint32_t COffset;
      BlockType ResType;
    } TryBlock;
  } Data;
  uint32_t Offset = 0;
  OpCode Code = OpCode::End;
  struct {
    bool IsAllocLabelList : 1;
    bool IsAllocValTypeList : 1;
    bool IsAllocJumpCatchList : 1;
    bool IsTryLast : 1;
    bool IsCatchLast : 1;
    bool IsDelegate : 1;
  } Flags;
  /// @}
};

// Type aliasing
using InstrVec = std::vector<Instruction>;
using InstrView = Span<const Instruction>;

} // namespace AST
} // namespace WasmEdge
