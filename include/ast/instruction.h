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
#include <cstddef>
#include <iterator>
#include <vector>

namespace WasmEdge {
namespace AST {

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

union InstructionData {
  struct {
    uint32_t JumpEnd;
    uint32_t JumpElse;
    BlockType ResType;
  } Blocks;
  struct {
    uint32_t TargetIdx;
    uint32_t SourceIdx;
    uint32_t StackOffset;
  } Indices;
  JumpDescriptor Jump;
  struct {
    uint32_t LabelListSize;
    JumpDescriptor *LabelList;
  } BrTable;
  ValType VType;
  struct {
    uint32_t ValTypeListSize;
    ValType *ValTypeList;
  } SelectT;
  struct {
    uint32_t TargetIdx;
    uint32_t MemAlign;
    uint64_t MemOffset;
  } Memories;
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
  uint128_t Num;
#else
  struct {
    uint64_t Low;
    uint64_t High;
  } Num;
#endif
  struct {
    bool IsExprLast : 1;
    bool IsTryBlockLast : 1;
  } EndFlags;
  BrCastDescriptor *BrCast;
  TryDescriptor *TryCatch;
};

struct InstructionFlags {
  uint8_t MemLane;
  bool IsAllocLabelList : 1;
  bool IsAllocValTypeList : 1;
  bool IsAllocBrCast : 1;
  bool IsAllocTryCatch : 1;
  bool IsViewOnly : 1;
};

class CompressedInstrVec;
class InstrIterator;

class Instruction {
  friend class CompressedInstrVec;

public:
  using JumpDescriptor = AST::JumpDescriptor;
  using BrCastDescriptor = AST::BrCastDescriptor;
  using CatchDescriptor = AST::CatchDescriptor;
  using TryDescriptor = AST::TryDescriptor;

  Instruction(OpCode Byte, uint32_t Off = 0) noexcept
      : Offset(Off), Code(Byte) {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
    Data.Num = static_cast<uint128_t>(0);
#else
    Data.Num.Low = static_cast<uint64_t>(0);
    Data.Num.High = static_cast<uint64_t>(0);
#endif
    Flags.IsAllocLabelList = false;
    Flags.IsAllocValTypeList = false;
    Flags.IsAllocBrCast = false;
    Flags.IsAllocTryCatch = false;
    Flags.IsViewOnly = false;
  }

  Instruction(OpCode C, uint32_t Off, InstructionData D,
              InstructionFlags F) noexcept
      : Data(D), Offset(Off), Code(C), Flags(F) {
    Flags.IsViewOnly = true;
  }

  Instruction(const Instruction &Instr) noexcept
      : Data(Instr.Data), Offset(Instr.Offset), Code(Instr.Code),
        Flags(Instr.Flags) {
    if (Flags.IsViewOnly) {
      return;
    }
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

  Instruction(Instruction &&Instr) noexcept
      : Data(Instr.Data), Offset(Instr.Offset), Code(Instr.Code),
        Flags(Instr.Flags) {
    Instr.Flags.IsAllocLabelList = false;
    Instr.Flags.IsAllocValTypeList = false;
    Instr.Flags.IsAllocBrCast = false;
    Instr.Flags.IsAllocTryCatch = false;
  }

  ~Instruction() { reset(); }

  Instruction &operator=(const Instruction &Instr) {
    if (this != &Instr) {
      Instruction Tmp(Instr);
      Tmp.swap(*this);
    }
    return *this;
  }

  OpCode getOpCode() const noexcept { return Code; }
  uint32_t getOffset() const noexcept { return Offset; }

  const BlockType &getBlockType() const noexcept { return Data.Blocks.ResType; }
  BlockType &getBlockType() noexcept { return Data.Blocks.ResType; }

  uint32_t getJumpEnd() const noexcept { return Data.Blocks.JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) noexcept { Data.Blocks.JumpEnd = Cnt; }

  uint32_t getJumpElse() const noexcept { return Data.Blocks.JumpElse; }
  void setJumpElse(const uint32_t Cnt) noexcept { Data.Blocks.JumpElse = Cnt; }

  const ValType &getValType() const noexcept { return Data.VType; }
  void setValType(const ValType &VType) noexcept { Data.VType = VType; }

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

  bool isExprLast() const noexcept { return Data.EndFlags.IsExprLast; }
  void setExprLast(bool Last = true) noexcept {
    Data.EndFlags.IsExprLast = Last;
  }

  bool isTryBlockLast() const noexcept { return Data.EndFlags.IsTryBlockLast; }
  void setTryBlockLast(bool Last = true) noexcept {
    Data.EndFlags.IsTryBlockLast = Last;
  }

  const JumpDescriptor &getJump() const noexcept { return Data.Jump; }
  JumpDescriptor &getJump() noexcept { return Data.Jump; }

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

  uint32_t getTargetIndex() const noexcept { return Data.Indices.TargetIdx; }
  uint32_t &getTargetIndex() noexcept { return Data.Indices.TargetIdx; }

  uint32_t getSourceIndex() const noexcept { return Data.Indices.SourceIdx; }
  uint32_t &getSourceIndex() noexcept { return Data.Indices.SourceIdx; }

  uint32_t getStackOffset() const noexcept { return Data.Indices.StackOffset; }
  uint32_t &getStackOffset() noexcept { return Data.Indices.StackOffset; }

  uint32_t getMemoryAlign() const noexcept { return Data.Memories.MemAlign; }
  uint32_t &getMemoryAlign() noexcept { return Data.Memories.MemAlign; }

  uint64_t getMemoryOffset() const noexcept { return Data.Memories.MemOffset; }
  uint64_t &getMemoryOffset() noexcept { return Data.Memories.MemOffset; }

  uint8_t getMemoryLane() const noexcept { return Flags.MemLane; }
  uint8_t &getMemoryLane() noexcept { return Flags.MemLane; }

  ValVariant getNum() const noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
    return ValVariant(Data.Num);
#else
    uint128_t N{Data.Num.High, Data.Num.Low};
    return ValVariant(N);
#endif
  }
  void setNum(ValVariant N) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
    Data.Num = N.get<uint128_t>();
#else
    uint128_t V = N.get<uint128_t>();
    Data.Num.Low = V.low();
    Data.Num.High = V.high();
#endif
  }

  void setBrCast(uint32_t LabelIdx) {
    reset();
    Data.BrCast = new BrCastDescriptor();
    Data.BrCast->Jump.TargetIndex = LabelIdx;
    Flags.IsAllocBrCast = true;
  }
  const BrCastDescriptor &getBrCast() const noexcept { return *Data.BrCast; }
  BrCastDescriptor &getBrCast() noexcept { return *Data.BrCast; }

  void setTryCatch() {
    reset();
    Data.TryCatch = new TryDescriptor();
    Flags.IsAllocTryCatch = true;
  }
  const TryDescriptor &getTryCatch() const noexcept { return *Data.TryCatch; }
  TryDescriptor &getTryCatch() noexcept { return *Data.TryCatch; }

private:
  void reset() noexcept {
    if (Flags.IsViewOnly) {
      return;
    }
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
    Flags.IsViewOnly = false;
  }

  void swap(Instruction &Instr) noexcept {
    std::swap(Data, Instr.Data);
    std::swap(Offset, Instr.Offset);
    std::swap(Code, Instr.Code);
    std::swap(Flags, Instr.Flags);
  }

  InstructionData Data;
  uint32_t Offset = 0;
  OpCode Code = OpCode::End;
  InstructionFlags Flags;
};

class CompressedInstrVec {
public:
  std::vector<OpCode> OpCodes;
  std::vector<uint32_t> Offsets;
  std::vector<InstructionFlags> Flags;
  std::vector<InstructionData> Immediates;
  std::vector<uint32_t> ImmediateIndices;

  class Proxy {
  public:
    Proxy(CompressedInstrVec *V, uint32_t Idx)
        : Vec(V), Index(Idx), Temp(V->getInstruction(Idx)) {}

    Proxy(const Proxy &P) = delete;
    Proxy(Proxy &&P) : Vec(P.Vec), Index(P.Index), Temp(P.Temp) {
      P.Vec = nullptr;
    }

    ~Proxy() {
      if (Vec) {
        Vec->OpCodes[Index] = Temp.Code;
        Vec->Offsets[Index] = Temp.Offset;
        Vec->Flags[Index] = Temp.Flags;
        if (Vec->ImmediateIndices[Index + 1] > Vec->ImmediateIndices[Index]) {
          Vec->Immediates[Vec->ImmediateIndices[Index]] = Temp.Data;
        }
      }
    }

    operator Instruction &() { return Temp; }
    operator const Instruction &() const { return Temp; }

    Instruction get() const { return Temp; }

    void setJumpElse(uint32_t Cnt) { Temp.setJumpElse(Cnt); }
    void setJumpEnd(uint32_t Cnt) { Temp.setJumpEnd(Cnt); }
    void setTryBlockLast(bool Last = true) { Temp.setTryBlockLast(Last); }
    void setExprLast(bool Last = true) { Temp.setExprLast(Last); }
    void setOffset(uint32_t Off) { Temp.Offset = Off; }
    void setFlags(const InstructionFlags &F) { Temp.Flags = F; }
    uint32_t getJumpElse() const { return Temp.getJumpElse(); }
    TryDescriptor &getTryCatch() { return Temp.getTryCatch(); }

  private:
    CompressedInstrVec *Vec;
    uint32_t Index;
    Instruction Temp;
  };

  CompressedInstrVec() { ImmediateIndices.push_back(0); }

  Proxy operator[](size_t Index) {
    return Proxy(this, static_cast<uint32_t>(Index));
  }
  Instruction operator[](size_t Index) const {
    return getInstruction(static_cast<uint32_t>(Index));
  }

  Proxy back() {
    return Proxy(this, static_cast<uint32_t>(OpCodes.size() - 1));
  }
  Instruction back() const {
    return getInstruction(static_cast<uint32_t>(OpCodes.size() - 1));
  }

  Instruction getInstruction(uint32_t Index) const {
    OpCode Code = OpCodes[Index];
    uint32_t Offset = Offsets[Index];
    InstructionFlags Flag = Flags[Index];
    InstructionData Data;

    uint32_t ImmedIdx = ImmediateIndices[Index];
    uint32_t NextImmedIdx = ImmediateIndices[Index + 1];

    if (NextImmedIdx > ImmedIdx) {
      Data = Immediates[ImmedIdx];
    } else {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
      Data.Num = static_cast<uint128_t>(0);
#else
      Data.Num.Low = 0;
      Data.Num.High = 0;
#endif
    }

    return Instruction(Code, Offset, Data, Flag);
  }

  void push_back(const Instruction &Instr) {
    OpCodes.push_back(Instr.Code);
    Offsets.push_back(Instr.Offset);
    Flags.push_back(Instr.Flags);

    bool HasImmediate = true;
    switch (Instr.Code) {
    case OpCode::Unreachable:
    case OpCode::Nop:
    case OpCode::Return:
    case OpCode::Drop:
    case OpCode::Select:
    case OpCode::End:
    case OpCode::Else:
    case OpCode::Ref__is_null:
    case OpCode::I32__eqz:
    case OpCode::I32__eq:
    case OpCode::I32__ne:
    case OpCode::I32__lt_s:
    case OpCode::I32__lt_u:
    case OpCode::I32__gt_s:
    case OpCode::I32__gt_u:
    case OpCode::I32__le_s:
    case OpCode::I32__le_u:
    case OpCode::I32__ge_s:
    case OpCode::I32__ge_u:
    case OpCode::I64__eqz:
    case OpCode::I64__eq:
    case OpCode::I64__ne:
    case OpCode::I64__lt_s:
    case OpCode::I64__lt_u:
    case OpCode::I64__gt_s:
    case OpCode::I64__gt_u:
    case OpCode::I64__le_s:
    case OpCode::I64__le_u:
    case OpCode::I64__ge_s:
    case OpCode::I64__ge_u:
    case OpCode::F32__eq:
    case OpCode::F32__ne:
    case OpCode::F32__lt:
    case OpCode::F32__gt:
    case OpCode::F32__le:
    case OpCode::F32__ge:
    case OpCode::F64__eq:
    case OpCode::F64__ne:
    case OpCode::F64__lt:
    case OpCode::F64__gt:
    case OpCode::F64__le:
    case OpCode::F64__ge:
    case OpCode::I32__clz:
    case OpCode::I32__ctz:
    case OpCode::I32__popcnt:
    case OpCode::I32__add:
    case OpCode::I32__sub:
    case OpCode::I32__mul:
    case OpCode::I32__div_s:
    case OpCode::I32__div_u:
    case OpCode::I32__rem_s:
    case OpCode::I32__rem_u:
    case OpCode::I32__and:
    case OpCode::I32__or:
    case OpCode::I32__xor:
    case OpCode::I32__shl:
    case OpCode::I32__shr_s:
    case OpCode::I32__shr_u:
    case OpCode::I32__rotl:
    case OpCode::I32__rotr:
    case OpCode::I64__clz:
    case OpCode::I64__ctz:
    case OpCode::I64__popcnt:
    case OpCode::I64__add:
    case OpCode::I64__sub:
    case OpCode::I64__mul:
    case OpCode::I64__div_s:
    case OpCode::I64__div_u:
    case OpCode::I64__rem_s:
    case OpCode::I64__rem_u:
    case OpCode::I64__and:
    case OpCode::I64__or:
    case OpCode::I64__xor:
    case OpCode::I64__shl:
    case OpCode::I64__shr_s:
    case OpCode::I64__shr_u:
    case OpCode::I64__rotl:
    case OpCode::I64__rotr:
    case OpCode::F32__abs:
    case OpCode::F32__neg:
    case OpCode::F32__ceil:
    case OpCode::F32__floor:
    case OpCode::F32__trunc:
    case OpCode::F32__nearest:
    case OpCode::F32__sqrt:
    case OpCode::F32__add:
    case OpCode::F32__sub:
    case OpCode::F32__mul:
    case OpCode::F32__div:
    case OpCode::F32__min:
    case OpCode::F32__max:
    case OpCode::F32__copysign:
    case OpCode::F64__abs:
    case OpCode::F64__neg:
    case OpCode::F64__ceil:
    case OpCode::F64__floor:
    case OpCode::F64__trunc:
    case OpCode::F64__nearest:
    case OpCode::F64__sqrt:
    case OpCode::F64__add:
    case OpCode::F64__sub:
    case OpCode::F64__mul:
    case OpCode::F64__div:
    case OpCode::F64__min:
    case OpCode::F64__max:
    case OpCode::F64__copysign:
    case OpCode::I32__wrap_i64:
    case OpCode::I32__trunc_f32_s:
    case OpCode::I32__trunc_f32_u:
    case OpCode::I32__trunc_f64_s:
    case OpCode::I32__trunc_f64_u:
    case OpCode::I64__extend_i32_s:
    case OpCode::I64__extend_i32_u:
    case OpCode::I64__trunc_f32_s:
    case OpCode::I64__trunc_f32_u:
    case OpCode::I64__trunc_f64_s:
    case OpCode::I64__trunc_f64_u:
    case OpCode::F32__convert_i32_s:
    case OpCode::F32__convert_i32_u:
    case OpCode::F32__convert_i64_s:
    case OpCode::F32__convert_i64_u:
    case OpCode::F32__demote_f64:
    case OpCode::F64__convert_i32_s:
    case OpCode::F64__convert_i32_u:
    case OpCode::F64__convert_i64_s:
    case OpCode::F64__convert_i64_u:
    case OpCode::F64__promote_f32:
    case OpCode::I32__reinterpret_f32:
    case OpCode::I64__reinterpret_f64:
    case OpCode::F32__reinterpret_i32:
    case OpCode::F64__reinterpret_i64:
    case OpCode::I32__extend8_s:
    case OpCode::I32__extend16_s:
    case OpCode::I64__extend8_s:
    case OpCode::I64__extend16_s:
    case OpCode::I64__extend32_s:
      HasImmediate = false;
      break;
    default:
      HasImmediate = true;
    }

    if (HasImmediate) {
      Immediates.push_back(Instr.Data);
    }
    ImmediateIndices.push_back(static_cast<uint32_t>(Immediates.size()));
  }

  template <typename... Args> void emplace_back(Args &&...args) {
    push_back(Instruction(std::forward<Args>(args)...));
  }

  void reserve(size_t Size) {
    OpCodes.reserve(Size);
    Offsets.reserve(Size);
    Flags.reserve(Size);
    ImmediateIndices.reserve(Size + 1);
  }

  template <typename It> void assign(It Begin, It End) {
    clear();
    for (auto I = Begin; I != End; ++I) {
      push_back(*I);
    }
  }

  void clear() {
    OpCodes.clear();
    Offsets.clear();
    Flags.clear();
    Immediates.clear();
    ImmediateIndices.clear();
    ImmediateIndices.push_back(0);
  }

  size_t size() const noexcept { return OpCodes.size(); }
};

class InstrIterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = Instruction;
  using difference_type = std::ptrdiff_t;
  using pointer = Instruction *;
  using reference = Instruction;

  class ProxyPointer {
    Instruction Val;

  public:
    ProxyPointer(Instruction V) : Val(V) {}
    Instruction *operator->() { return &Val; }
    const Instruction *operator->() const { return &Val; }
  };

  InstrIterator() noexcept : Vec(nullptr), Index(0) {}
  InstrIterator(std::nullptr_t) noexcept : Vec(nullptr), Index(0) {}
  InstrIterator(const CompressedInstrVec *V, uint32_t Idx) noexcept
      : Vec(V), Index(Idx) {}

  Instruction operator*() const { return Vec->getInstruction(Index); }

  ProxyPointer operator->() const {
    return ProxyPointer(Vec->getInstruction(Index));
  }

  InstrIterator &operator++() noexcept {
    ++Index;
    return *this;
  }
  InstrIterator operator++(int) noexcept {
    InstrIterator Tmp = *this;
    ++Index;
    return Tmp;
  }
  InstrIterator &operator--() noexcept {
    --Index;
    return *this;
  }
  InstrIterator operator--(int) noexcept {
    InstrIterator Tmp = *this;
    --Index;
    return Tmp;
  }

  InstrIterator &operator+=(difference_type N) noexcept {
    Index += N;
    return *this;
  }
  InstrIterator operator+(difference_type N) const noexcept {
    return InstrIterator(Vec, Index + N);
  }
  InstrIterator &operator-=(difference_type N) noexcept {
    Index -= N;
    return *this;
  }
  InstrIterator operator-(difference_type N) const noexcept {
    return InstrIterator(Vec, Index - N);
  }
  difference_type operator-(const InstrIterator &Other) const noexcept {
    return Index - Other.Index;
  }

  bool operator==(const InstrIterator &Other) const noexcept {
    return Index == Other.Index && Vec == Other.Vec;
  }
  bool operator!=(const InstrIterator &Other) const noexcept {
    return !(*this == Other);
  }
  bool operator<(const InstrIterator &Other) const noexcept {
    return Index < Other.Index;
  }
  bool operator<=(const InstrIterator &Other) const noexcept {
    return Index <= Other.Index;
  }
  bool operator>(const InstrIterator &Other) const noexcept {
    return Index > Other.Index;
  }
  bool operator>=(const InstrIterator &Other) const noexcept {
    return Index >= Other.Index;
  }

private:
  const CompressedInstrVec *Vec;
  uint32_t Index;
};

class InstrView {
public:
  using iterator = InstrIterator;
  using const_iterator = InstrIterator;
  using value_type = Instruction;

  InstrView() : B(nullptr), E(nullptr) {}
  InstrView(std::nullptr_t) : B(nullptr), E(nullptr) {}
  InstrView(InstrIterator Begin, InstrIterator End) : B(Begin), E(End) {}
  InstrView(const CompressedInstrVec &V) : B(&V, 0), E(&V, V.size()) {}

  InstrIterator begin() const { return B; }
  InstrIterator end() const { return E; }
  InstrIterator cbegin() const { return B; }
  InstrIterator cend() const { return E; }

  size_t size() const { return E - B; }
  bool empty() const { return B == E; }

  Instruction operator[](size_t Idx) const { return *(B + Idx); }

private:
  InstrIterator B;
  InstrIterator E;
};

using InstrVec = CompressedInstrVec;

} // namespace AST
} // namespace WasmEdge
// Temporary size check - remove before final PR
static_assert(sizeof(Instruction) <= 24,
              "Instruction view is still too large!");