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
    int32_t PCOffset;
  };
  struct BrCastDescriptor {
    struct JumpDescriptor Jump;
    ValType RType1, RType2;
  };
  struct CatchDescriptor {
    bool IsAll : 1;
    bool IsRef : 1;
    uint32_t TagIndex;
    uint32_t LabelIndex;
    struct JumpDescriptor Jump;
  };
  struct TryDescriptor {
    BlockType ResType;
    uint32_t BlockParamNum;
    uint32_t JumpEnd;
    std::vector<CatchDescriptor> Catch;
  };

public:
  /// Constructor assigns the OpCode and the Offset.
  Instruction(OpCode Code, uint32_t Off = 0) noexcept {
    std::fill_n(Raw, 8, 0U);
    Raw[5] = Off;
    Raw[6] = static_cast<uint32_t>(Code);
  }

  /// Copy constructor.
  Instruction(const Instruction &Instr) noexcept {
    std::copy_n(Instr.Raw, 8, Raw);
    const auto &F = getFlags();
    const auto &SrcData = Instr.getInner();
    auto &Data = getInner();
    if (F.IsAllocLabelList) {
      Data.BrTable.LabelList = new JumpDescriptor[Data.BrTable.LabelListSize];
      std::copy_n(SrcData.BrTable.LabelList, Data.BrTable.LabelListSize,
                  Data.BrTable.LabelList);
    } else if (F.IsAllocValTypeList) {
      Data.SelectT.ValTypeList = new ValType[Data.SelectT.ValTypeListSize];
      std::copy_n(SrcData.SelectT.ValTypeList, Data.SelectT.ValTypeListSize,
                  Data.SelectT.ValTypeList);
    } else if (F.IsAllocBrCast) {
      Data.BrCast = new BrCastDescriptor(*SrcData.BrCast);
    } else if (F.IsAllocTryCatch) {
      Data.TryCatch = new TryDescriptor(*SrcData.TryCatch);
    }
  }

  /// Move constructor.
  Instruction(Instruction &&Instr) noexcept {
    std::copy_n(Instr.Raw, 8, Raw);
    Instr.Raw[7] = 0;
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
  OpCode getOpCode() const noexcept { return static_cast<OpCode>(Raw[6]); }

  /// Getter of Offset.
  uint32_t getOffset() const noexcept { return Raw[5]; }

  /// Getter and setter of block type.
  const BlockType &getBlockType() const noexcept {
    return getInner().Blocks.ResType;
  }
  BlockType &getBlockType() noexcept { return getInner().Blocks.ResType; }

  /// Getter and setter of jump count to End instruction.
  uint32_t getJumpEnd() const noexcept { return getInner().Blocks.JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) noexcept {
    getInner().Blocks.JumpEnd = Cnt;
  }

  /// Getter and setter of jump count to Else instruction.
  uint32_t getJumpElse() const noexcept { return getInner().Blocks.JumpElse; }
  void setJumpElse(const uint32_t Cnt) noexcept {
    getInner().Blocks.JumpElse = Cnt;
  }

  /// Getter and setter of value type.
  const ValType &getValType() const noexcept { return getInner().VType; }
  void setValType(const ValType &VType) noexcept { getInner().VType = VType; }

  /// Getter and setter of label list.
  void setLabelListSize(uint32_t Size) {
    reset();
    if (Size > 0) {
      getInner().BrTable.LabelListSize = Size;
      getInner().BrTable.LabelList = new JumpDescriptor[Size];
      getFlags().IsAllocLabelList = true;
    }
  }
  Span<const JumpDescriptor> getLabelList() const noexcept {
    return Span<const JumpDescriptor>(
        getInner().BrTable.LabelList,
        getFlags().IsAllocLabelList ? getInner().BrTable.LabelListSize : 0);
  }
  Span<JumpDescriptor> getLabelList() noexcept {
    return Span<JumpDescriptor>(
        getInner().BrTable.LabelList,
        getFlags().IsAllocLabelList ? getInner().BrTable.LabelListSize : 0);
  }

  /// Getter and setter of expression end for End instruction.
  bool isExprLast() const noexcept { return getInner().EndFlags.IsExprLast; }
  void setExprLast(bool Last = true) noexcept {
    getInner().EndFlags.IsExprLast = Last;
  }

  /// Getter and setter of try block end for End instruction.
  bool isTryBlockLast() const noexcept {
    return getInner().EndFlags.IsTryBlockLast;
  }
  void setTryBlockLast(bool Last = true) noexcept {
    getInner().EndFlags.IsTryBlockLast = Last;
  }

  /// Getter and setter of Jump for Br* instruction.
  void setJump(uint32_t LabelIdx) { getInner().Jump.TargetIndex = LabelIdx; }
  const JumpDescriptor &getJump() const noexcept { return getInner().Jump; }
  JumpDescriptor &getJump() noexcept { return getInner().Jump; }

  /// Getter and setter of selecting value types list.
  void setValTypeListSize(uint32_t Size) {
    reset();
    if (Size > 0) {
      getInner().SelectT.ValTypeListSize = Size;
      getInner().SelectT.ValTypeList = new ValType[Size];
      getFlags().IsAllocValTypeList = true;
    }
  }
  Span<const ValType> getValTypeList() const noexcept {
    return Span<const ValType>(getInner().SelectT.ValTypeList,
                               getInner().SelectT.ValTypeListSize);
  }
  Span<ValType> getValTypeList() noexcept {
    return Span<ValType>(getInner().SelectT.ValTypeList,
                         getInner().SelectT.ValTypeListSize);
  }

  /// Getter and setter of target index.
  uint32_t getTargetIndex() const noexcept {
    return getInner().Indices.TargetIdx;
  }
  uint32_t &getTargetIndex() noexcept { return getInner().Indices.TargetIdx; }

  /// Getter and setter of source index.
  uint32_t getSourceIndex() const noexcept {
    return getInner().Indices.SourceIdx;
  }
  uint32_t &getSourceIndex() noexcept { return getInner().Indices.SourceIdx; }

  /// Getter and setter of stack offset.
  uint32_t getStackOffset() const noexcept {
    return getInner().Indices.StackOffset;
  }
  uint32_t &getStackOffset() noexcept { return getInner().Indices.StackOffset; }

  /// Getter and setter of memory alignment.
  uint32_t getMemoryAlign() const noexcept {
    return getInner().Memories.MemAlign;
  }
  uint32_t &getMemoryAlign() noexcept { return getInner().Memories.MemAlign; }

  /// Getter of memory offset.
  uint32_t getMemoryOffset() const noexcept {
    return getInner().Memories.MemOffset;
  }
  uint32_t &getMemoryOffset() noexcept { return getInner().Memories.MemOffset; }

  /// Getter of memory lane.
  uint8_t getMemoryLane() const noexcept { return getInner().Memories.MemLane; }
  uint8_t &getMemoryLane() noexcept { return getInner().Memories.MemLane; }

  /// Getter and setter of the constant value.
  ValVariant getNum() const noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    return ValVariant(getInner().Num);
#else
    uint128_t N(getInner().Num.High, getInner().Num.Low);
    return ValVariant(N);
#endif
  }
  void setNum(ValVariant N) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    getInner().Num = N.get<uint128_t>();
#else
    std::memcpy(&(getInner().Num), &N.get<uint128_t>(), sizeof(uint128_t));
#endif
  }

  /// Getter and setter of BrCast info for Br_cast instructions.
  void setBrCast(uint32_t LabelIdx) {
    reset();
    getInner().BrCast = new BrCastDescriptor();
    getInner().BrCast->Jump.TargetIndex = LabelIdx;
    getFlags().IsAllocBrCast = true;
  }
  const BrCastDescriptor &getBrCast() const noexcept {
    return *getInner().BrCast;
  }
  BrCastDescriptor &getBrCast() noexcept { return *getInner().BrCast; }

  /// Getter and setter of try block info for try_table instruction.
  void setTryCatch() {
    reset();
    getInner().TryCatch = new TryDescriptor();
    getFlags().IsAllocTryCatch = true;
  }
  const TryDescriptor &getTryCatch() const noexcept {
    return *getInner().TryCatch;
  }
  TryDescriptor &getTryCatch() noexcept { return *getInner().TryCatch; }

private:
  /// Release allocated resources.
  void reset() noexcept {
    if (getFlags().IsAllocLabelList) {
      getInner().BrTable.LabelListSize = 0;
      delete[] getInner().BrTable.LabelList;
    } else if (getFlags().IsAllocValTypeList) {
      getInner().SelectT.ValTypeListSize = 0;
      delete[] getInner().SelectT.ValTypeList;
    } else if (getFlags().IsAllocBrCast) {
      delete getInner().BrCast;
    } else if (getFlags().IsAllocTryCatch) {
      delete getInner().TryCatch;
    }
    getFlags().IsAllocLabelList = false;
    getFlags().IsAllocValTypeList = false;
    getFlags().IsAllocBrCast = false;
    getFlags().IsAllocTryCatch = false;
  }

  /// Swap function.
  void swap(Instruction &Instr) noexcept { std::swap(Raw, Instr.Raw); }

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
    // Type 5: ValType.
    ValType VType;
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
    // Type 9: End flags.
    struct {
      bool IsExprLast : 1;
      bool IsTryBlockLast : 1;
    } EndFlags;
    // Type 10: TypeCastBranch.
    BrCastDescriptor *BrCast;
    // Type 11: Try Block.
    TryDescriptor *TryCatch;
  };

  union Inner &getInner() noexcept {
    return reinterpret_cast<union Inner &>(Raw[0]);
  }
  const union Inner &getInner() const noexcept {
    return reinterpret_cast<const union Inner &>(Raw[0]);
  }

  struct Flags {
    bool IsAllocLabelList : 1;
    bool IsAllocValTypeList : 1;
    bool IsAllocBrCast : 1;
    bool IsAllocTryCatch : 1;
  };

  struct Flags &getFlags() noexcept {
    return reinterpret_cast<struct Flags &>(Raw[7]);
  }
  const struct Flags &getFlags() const noexcept {
    return reinterpret_cast<const struct Flags &>(Raw[7]);
  }

  // Raw data in 8 uint32_t length.
  // 0th - 4th: Instruction immediate.
  // 5th: Offset.
  // 6th: OpCode.
  // 7th: Flags.
  uint32_t Raw[8];
  /// @}
};

// Type aliasing
using InstrVec = std::vector<Instruction>;
using InstrView = Span<const Instruction>;

} // namespace AST
} // namespace WasmEdge
