// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
    uint32_t StackEraseBegin;
    uint32_t StackEraseEnd;
    int32_t PCOffset;
  };
  struct BrCastDescriptor {
    struct JumpDescriptor Jump;
    ValType RType1, RType2;
  };
  struct CatchDescriptor {
    // LEGACY-EH: remove this flag after deprecating legacy EH.
    bool IsLegacy : 1;
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
  // LEGACY-EH: remove this struct after deprecating legacy EH.
  struct CatchDescriptorLegacy {
    uint32_t TagIndex;
    uint32_t LabelIndex;
    uint32_t CatchIndex;
    uint32_t CatchPCOffset;
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
    Flags.IsAllocLabelList = false;
    Flags.IsAllocValTypeList = false;
    Flags.IsAllocBrCast = false;
    Flags.IsAllocTryCatch = false;
  }

  /// Copy constructor.
  Instruction(const Instruction &Instr) noexcept
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
    } else if (Flags.IsAllocBrCast) {
      Data.BrCast = new BrCastDescriptor(*Instr.Data.BrCast);
    } else if (Flags.IsAllocTryCatch) {
      Data.TryCatch = new TryDescriptor(*Instr.Data.TryCatch);
    }
  }

  /// Move constructor.
  Instruction(Instruction &&Instr) noexcept
      : Data(Instr.Data), Offset(Instr.Offset), Code(Instr.Code),
        Flags(Instr.Flags) {
    Instr.Flags.IsAllocLabelList = false;
    Instr.Flags.IsAllocValTypeList = false;
    Instr.Flags.IsAllocBrCast = false;
    Instr.Flags.IsAllocTryCatch = false;
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
  const BlockType &getBlockType() const noexcept { return Data.Blocks.ResType; }
  BlockType &getBlockType() noexcept { return Data.Blocks.ResType; }

  /// Getter and setter of jump count to End instruction.
  uint32_t getJumpEnd() const noexcept { return Data.Blocks.JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) noexcept { Data.Blocks.JumpEnd = Cnt; }

  /// Getter and setter of jump count to Else instruction.
  uint32_t getJumpElse() const noexcept { return Data.Blocks.JumpElse; }
  void setJumpElse(const uint32_t Cnt) noexcept { Data.Blocks.JumpElse = Cnt; }

  /// Getter and setter of value type.
  const ValType &getValType() const noexcept { return Data.VType; }
  void setValType(const ValType &VType) noexcept { Data.VType = VType; }

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

  /// Getter and setter of expression end for End instruction.
  bool isExprLast() const noexcept { return Data.EndFlags.IsExprLast; }
  void setExprLast(bool Last = true) noexcept {
    Data.EndFlags.IsExprLast = Last;
  }

  /// Getter and setter of try block end for End instruction.
  bool isTryBlockLast() const noexcept { return Data.EndFlags.IsTryBlockLast; }
  void setTryBlockLast(bool Last = true) noexcept {
    Data.EndFlags.IsTryBlockLast = Last;
  }

  // LEGACY-EH: remove these functions after deprecating legacy EH.
  /// Getter and setter of try block end for End instruction.
  bool isLegacyTryBlockLast() const noexcept {
    return Data.EndFlags.IsLegacyTryBlockLast;
  }
  void setLegacyTryBlockLast(bool Last = true) noexcept {
    Data.EndFlags.IsLegacyTryBlockLast = Last;
  }

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

  // LEGACY-EH: remove these functions after deprecating legacy EH.
  /// Getter and setter of legacy Catch for Catch* instructions.
  const CatchDescriptorLegacy &getCatchLegacy() const noexcept {
    return Data.CatchLegacy;
  }
  CatchDescriptorLegacy &getCatchLegacy() noexcept { return Data.CatchLegacy; }

  /// Getter and setter of the constant value.
  ValVariant getNum() const noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    return ValVariant(Data.Num);
#else
    uint128_t N{Data.Num.High, Data.Num.Low};
    return ValVariant(N);
#endif
  }
  void setNum(ValVariant N) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
    Data.Num = N.get<uint128_t>();
#else
    uint128_t V = N.get<uint128_t>();
    Data.Num.Low = V.low();
    Data.Num.High = V.high();
#endif
  }

  /// Getter and setter of BrCast info for Br_cast instructions.
  void setBrCast(uint32_t LabelIdx) {
    reset();
    Data.BrCast = new BrCastDescriptor();
    Data.BrCast->Jump.TargetIndex = LabelIdx;
    Flags.IsAllocBrCast = true;
  }
  const BrCastDescriptor &getBrCast() const noexcept { return *Data.BrCast; }
  BrCastDescriptor &getBrCast() noexcept { return *Data.BrCast; }

  /// Getter and setter of try block info for try_table instruction.
  void setTryCatch() {
    reset();
    Data.TryCatch = new TryDescriptor();
    Flags.IsAllocTryCatch = true;
  }
  const TryDescriptor &getTryCatch() const noexcept { return *Data.TryCatch; }
  TryDescriptor &getTryCatch() noexcept { return *Data.TryCatch; }

private:
  /// Release allocated resources.
  void reset() noexcept {
    if (Flags.IsAllocLabelList) {
      Data.BrTable.LabelListSize = 0;
      delete[] Data.BrTable.LabelList;
    } else if (Flags.IsAllocValTypeList) {
      Data.SelectT.ValTypeListSize = 0;
      delete[] Data.SelectT.ValTypeList;
    } else if (Flags.IsAllocBrCast) {
      delete Data.BrCast;
    } else if (Flags.IsAllocTryCatch) {
      delete Data.TryCatch;
    }
    Flags.IsAllocLabelList = false;
    Flags.IsAllocValTypeList = false;
    Flags.IsAllocBrCast = false;
    Flags.IsAllocTryCatch = false;
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
      // LEGACY-EH: remove this flag after deprecating legacy EH.
      bool IsLegacyTryBlockLast : 1;
    } EndFlags;
    // Type 10: TypeCastBranch.
    BrCastDescriptor *BrCast;
    // Type 11: Try Block.
    TryDescriptor *TryCatch;
    // LEGACY-EH: remove this case after deprecating legacy EH.
    // Type 12: Legacy Catch descriptor.
    CatchDescriptorLegacy CatchLegacy;
  } Data;
  uint32_t Offset = 0;
  OpCode Code = OpCode::End;
  struct {
    bool IsAllocLabelList : 1;
    bool IsAllocValTypeList : 1;
    bool IsAllocBrCast : 1;
    bool IsAllocTryCatch : 1;
  } Flags;
  /// @}
};

// Type aliasing
using InstrVec = std::vector<Instruction>;
using InstrView = Span<const Instruction>;

} // namespace AST
} // namespace WasmEdge
