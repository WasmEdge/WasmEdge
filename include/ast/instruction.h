// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/ast/instruction.h - Instruction class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Instruction node class and the
/// flat instruction buffer of an expression.
///
/// The instructions of an expression are packed into one byte buffer of
/// variable-length records. Each record is a header followed by the
/// immediates of the instruction:
///
///   [OpCode:u16, Size:u16, Offset:u32, Immediates...]
///
/// The records and the immediates are 4-byte aligned. Every jump distance
/// (JumpDescriptor::PCOffset, JumpEnd, and JumpElse) is the byte offset from
/// the owning record to the target record.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_ast.hpp"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace AST {

class InstrIterator;
class InstrView;
class InstrVec;

/// Instruction node class. A view of one record in the instruction buffer.
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
    uint32_t CatchNum;

    /// Getter for the catch clause list, which follows this descriptor.
    Span<const CatchDescriptor> getCatchList() const noexcept {
      return Span<const CatchDescriptor>(
          reinterpret_cast<const CatchDescriptor *>(this + 1), CatchNum);
    }
    Span<CatchDescriptor> getCatchList() noexcept {
      return Span<CatchDescriptor>(
          reinterpret_cast<CatchDescriptor *>(this + 1), CatchNum);
    }
  };

  Instruction() noexcept = default;

  /// Getter for OpCode.
  OpCode getOpCode() const noexcept {
    return static_cast<OpCode>(getHeader()->Code);
  }

  /// Getter for Offset.
  uint32_t getOffset() const noexcept { return getHeader()->Offset; }

  /// Getter for block type.
  const BlockType &getBlockType() const noexcept {
    return getImm<BlocksImm>()->ResType;
  }

  /// Getter and setter for the byte distance to the End instruction.
  uint32_t getJumpEnd() const noexcept { return getImm<ElseImm>()->JumpEnd; }
  void setJumpEnd(const uint32_t Cnt) noexcept {
    getImm<ElseImm>()->JumpEnd = Cnt;
  }

  /// Getter and setter for the byte distance to the Else instruction.
  uint32_t getJumpElse() const noexcept {
    return getImm<BlocksImm>()->JumpElse;
  }
  void setJumpElse(const uint32_t Cnt) noexcept {
    getImm<BlocksImm>()->JumpElse = Cnt;
  }

  /// Getter for value type.
  const ValType &getValType() const noexcept { return *getImm<ValType>(); }

  /// Getter for label list. The default label is the last entry.
  Span<const JumpDescriptor> getLabelList() const noexcept {
    return getList<JumpDescriptor>();
  }
  Span<JumpDescriptor> getLabelList() noexcept {
    return getList<JumpDescriptor>();
  }

  /// Getter and setter for expression end for End instruction.
  bool isExprLast() const noexcept { return getImm<EndImm>()->IsExprLast; }
  void setExprLast(bool Last = true) noexcept {
    getImm<EndImm>()->IsExprLast = Last;
  }

  /// Getter and setter for the try block end for End instruction.
  bool isTryBlockLast() const noexcept {
    return getImm<EndImm>()->IsTryBlockLast;
  }
  void setTryBlockLast(bool Last = true) noexcept {
    getImm<EndImm>()->IsTryBlockLast = Last;
  }

  /// Getter for Jump for Br* instructions.
  const JumpDescriptor &getJump() const noexcept {
    return *getImm<JumpDescriptor>();
  }
  JumpDescriptor &getJump() noexcept { return *getImm<JumpDescriptor>(); }

  /// Getter for the selected value type list.
  Span<const ValType> getValTypeList() const noexcept {
    return getList<ValType>();
  }
  Span<ValType> getValTypeList() noexcept { return getList<ValType>(); }

  /// Getter for target index.
  uint32_t getTargetIndex() const noexcept {
    return getImm<IndexImm>()->TargetIdx;
  }

  /// Getter for source index.
  uint32_t getSourceIndex() const noexcept {
    return getImm<IndicesImm>()->SourceIdx;
  }

  /// Getter and setter for stack offset.
  uint32_t getStackOffset() const noexcept {
    return getImm<IndicesImm>()->SourceIdx;
  }
  uint32_t &getStackOffset() noexcept {
    return getImm<IndicesImm>()->SourceIdx;
  }

  /// Getter for memory alignment.
  uint32_t getMemoryAlign() const noexcept {
    return getImm<MemoryImm>()->MemAlign;
  }

  /// Getter for memory offset.
  uint64_t getMemoryOffset() const noexcept {
    uint64_t MemOffset;
    std::memcpy(&MemOffset, getImm<MemoryImm>()->MemOffset, sizeof(MemOffset));
    return MemOffset;
  }

  /// Getter for memory lane.
  uint8_t getMemoryLane() const noexcept {
    if (getHeader()->Size == sizeof(Header) + sizeof(LaneImm)) {
      return static_cast<uint8_t>(getImm<LaneImm>()->MemLane);
    }
    return static_cast<uint8_t>(getImm<MemoryLaneImm>()->MemLane);
  }

  /// Getter for the constant value. The scalar constants are zero-extended.
  ValVariant getNum() const noexcept {
    ValVariant Num = static_cast<uint128_t>(0U);
    if (getHeader()->Size == sizeof(Header) + sizeof(uint64_t)) {
      std::memcpy(static_cast<void *>(&Num), getImm<uint8_t>(),
                  sizeof(uint64_t));
    } else {
      std::memcpy(static_cast<void *>(&Num), getImm<uint8_t>(),
                  sizeof(uint128_t));
    }
    return Num;
  }

  /// Getter for BrCast info for Br_cast instructions.
  const BrCastDescriptor &getBrCast() const noexcept {
    return *getImm<BrCastDescriptor>();
  }
  BrCastDescriptor &getBrCast() noexcept { return *getImm<BrCastDescriptor>(); }

  /// Getter for try block info for try_table instruction.
  const TryDescriptor &getTryCatch() const noexcept {
    return *getImm<TryDescriptor>();
  }
  TryDescriptor &getTryCatch() noexcept { return *getImm<TryDescriptor>(); }

  /// Getter for the iterator at this instruction.
  inline InstrIterator getIterator() const noexcept;

private:
  friend class InstrIterator;
  friend class InstrView;
  friend class InstrVec;

  /// \name Layouts of the record header and the immediates.
  /// @{
  struct Header {
    uint16_t Code;
    uint16_t Size;
    uint32_t Offset;
  };
  // Blocks. The Else instruction keeps the leading JumpEnd only.
  struct ElseImm {
    uint32_t JumpEnd;
  };
  struct BlocksImm {
    uint32_t JumpEnd;
    uint32_t JumpElse;
    BlockType ResType;
  };
  // Indices. The SourceIdx is also the stack offset of the local instructions.
  struct IndexImm {
    uint32_t TargetIdx;
  };
  struct IndicesImm {
    uint32_t TargetIdx;
    uint32_t SourceIdx;
  };
  // Size of the label list or the value type list following it.
  struct ListImm {
    uint32_t Size;
  };
  // Memories and lanes. The 64-bit MemOffset is not aligned.
  struct MemoryImm {
    uint32_t TargetIdx;
    uint32_t MemAlign;
    uint8_t MemOffset[sizeof(uint64_t)];
  };
  struct MemoryLaneImm {
    MemoryImm Memory;
    uint32_t MemLane;
  };
  struct LaneImm {
    uint32_t MemLane;
  };
  // End flags.
  struct alignas(uint32_t) EndImm {
    bool IsExprLast : 1;
    bool IsTryBlockLast : 1;
  };
  /// @}

  /// The size field of a record exceeding it, which is sized by its list.
  static constexpr uint16_t OversizeMark = UINT16_MAX;

  explicit Instruction(const uint8_t *P) noexcept : Ptr(P) {}

  /// Getter for the header and the immediate of the record. The buffer is
  /// owned by a mutable vector, so the setters cast the constness away.
  const Header *getHeader() const noexcept {
    return reinterpret_cast<const Header *>(Ptr);
  }
  Header *getHeader() noexcept {
    return const_cast<Header *>(std::as_const(*this).getHeader());
  }
  template <typename T> const T *getImm() const noexcept {
    static_assert(std::is_trivially_copyable_v<T> &&
                  alignof(T) <= alignof(Header));
    return reinterpret_cast<const T *>(Ptr + sizeof(Header));
  }
  template <typename T> T *getImm() noexcept {
    return const_cast<T *>(std::as_const(*this).getImm<T>());
  }

  /// Getter for the immediate list, which follows its size.
  template <typename T> Span<const T> getList() const noexcept {
    static_assert(std::is_trivially_copyable_v<T> &&
                  alignof(T) <= alignof(ListImm));
    const auto *List = getImm<ListImm>();
    return Span<const T>(reinterpret_cast<const T *>(List + 1), List->Size);
  }
  template <typename T> Span<T> getList() noexcept {
    const auto List = std::as_const(*this).getList<T>();
    return Span<T>(const_cast<T *>(List.data()), List.size());
  }

  /// Getter for the record size in bytes.
  uint32_t getSize() const noexcept {
    const uint32_t Size = getHeader()->Size;
    if (likely(Size != OversizeMark)) {
      return Size;
    }
    return getListRecordSize();
  }

  /// Getter for the size of a record exceeding the size field, from its list.
  uint32_t getListRecordSize() const noexcept {
    switch (getOpCode()) {
    case OpCode::Br_table:
      return static_cast<uint32_t>(sizeof(Header) + sizeof(ListImm)) +
             static_cast<uint32_t>(sizeof(JumpDescriptor)) *
                 getImm<ListImm>()->Size;
    case OpCode::Select_t:
      return static_cast<uint32_t>(sizeof(Header) + sizeof(ListImm)) +
             static_cast<uint32_t>(sizeof(ValType)) * getImm<ListImm>()->Size;
    case OpCode::Try_table:
      return static_cast<uint32_t>(sizeof(Header) + sizeof(TryDescriptor)) +
             static_cast<uint32_t>(sizeof(CatchDescriptor)) *
                 getImm<TryDescriptor>()->CatchNum;
    default:
      assumingUnreachable();
    }
  }

  /// The record start address in the flat instruction buffer.
  const uint8_t *Ptr = nullptr;
};

/// Iterator of the flat instruction buffer. The increment steps to the next
/// record, and the arithmetic shifts by a byte distance as the jumps do.
class InstrIterator {
public:
  using iterator_category = std::input_iterator_tag;
  using value_type = Instruction;
  using difference_type = ptrdiff_t;
  using pointer = const Instruction *;
  using reference = const Instruction &;

  InstrIterator() noexcept = default;
  explicit InstrIterator(const Instruction &Instr) noexcept : Cur(Instr) {}

  const Instruction &operator*() const noexcept { return Cur; }
  const Instruction *operator->() const noexcept { return &Cur; }

  InstrIterator &operator++() noexcept {
    Cur = Instruction(Cur.Ptr + Cur.getSize());
    return *this;
  }
  InstrIterator operator++(int) noexcept {
    InstrIterator It = *this;
    ++*this;
    return It;
  }
  InstrIterator operator+(ptrdiff_t Bytes) const noexcept {
    return InstrIterator(Instruction(Cur.Ptr + Bytes));
  }
  ptrdiff_t operator-(const InstrIterator &It) const noexcept {
    return Cur.Ptr - It.Cur.Ptr;
  }

  bool operator==(const InstrIterator &It) const noexcept {
    return Cur.Ptr == It.Cur.Ptr;
  }
  bool operator!=(const InstrIterator &It) const noexcept {
    return Cur.Ptr != It.Cur.Ptr;
  }
  bool operator<(const InstrIterator &It) const noexcept {
    return Cur.Ptr < It.Cur.Ptr;
  }
  bool operator>(const InstrIterator &It) const noexcept {
    return Cur.Ptr > It.Cur.Ptr;
  }
  bool operator<=(const InstrIterator &It) const noexcept {
    return Cur.Ptr <= It.Cur.Ptr;
  }
  bool operator>=(const InstrIterator &It) const noexcept {
    return Cur.Ptr >= It.Cur.Ptr;
  }
  explicit operator bool() const noexcept { return Cur.Ptr != nullptr; }

private:
  Instruction Cur;
};

inline InstrIterator Instruction::getIterator() const noexcept {
  return InstrIterator(*this);
}

/// Non-owning view of the flat instruction buffer of an expression.
class InstrView {
public:
  using iterator = InstrIterator;

  InstrView() noexcept = default;
  InstrView(const InstrVec &Vec) noexcept;

  iterator begin() const noexcept { return iterator(Instruction(Begin)); }
  iterator end() const noexcept { return iterator(Instruction(End)); }
  bool empty() const noexcept { return Begin == End; }

  /// Getter for the instruction count, which walks the records.
  size_t size() const noexcept {
    size_t Cnt = 0;
    for (auto It = begin(); It != end(); ++It) {
      Cnt++;
    }
    return Cnt;
  }

  /// Getter for the instruction at the index, which walks the records.
  Instruction operator[](size_t Idx) const noexcept {
    auto It = begin();
    while (Idx-- > 0) {
      ++It;
    }
    return *It;
  }

  /// Getter for the last instruction, which is the End of the expression.
  Instruction back() const noexcept {
    assuming(!empty());
    return Instruction(
        End - (sizeof(Instruction::Header) + sizeof(Instruction::EndImm)));
  }

private:
  friend class InstrVec;

  const uint8_t *Begin = nullptr;
  const uint8_t *End = nullptr;
};

/// Owning flat instruction buffer of an expression. Each append emits one
/// record and returns its byte position, or `InvalidPos` over the size limit.
class InstrVec {
public:
  using iterator = InstrIterator;

  /// The buffer size limit in bytes, which the 32-bit jump distances cover.
  static inline constexpr const uint32_t MaxSize = INT32_MAX;
  static inline constexpr const uint32_t InvalidPos = UINT32_MAX;

  InstrVec() noexcept = default;

  /// Copy the records of a view. The spare byte keeps the end apart from the
  /// begin of any other buffer, as the end is the interpreter's stop position.
  explicit InstrVec(InstrView View) {
    Data.reserve(static_cast<size_t>(View.End - View.Begin) + 1);
    Data.assign(View.Begin, View.End);
  }

  iterator begin() const noexcept { return InstrView(*this).begin(); }
  iterator end() const noexcept { return InstrView(*this).end(); }
  bool empty() const noexcept { return Data.empty(); }
  size_t size() const noexcept { return InstrView(*this).size(); }
  Instruction operator[](size_t Idx) const noexcept {
    return InstrView(*this)[Idx];
  }
  Instruction back() const noexcept { return InstrView(*this).back(); }
  void clear() noexcept { Data.clear(); }

  /// Getter for the instruction record at the byte position.
  Instruction getInstr(uint32_t Pos) noexcept {
    return Instruction(Data.data() + Pos);
  }

  /// Append an instruction without immediates.
  uint32_t append(OpCode Code, uint32_t Offset = 0) {
    return appendHeader(Code, Offset, 0);
  }

  /// Append an End instruction with the flags cleared.
  uint32_t appendEnd(uint32_t Offset = 0) {
    return appendHeader(OpCode::End, Offset, sizeof(Instruction::EndImm));
  }

  /// Append an Else instruction. The jump distance is patched later.
  uint32_t appendElse(uint32_t Offset = 0) {
    return appendHeader(OpCode::Else, Offset, sizeof(Instruction::ElseImm));
  }

  /// Append a block instruction. The jump distances are patched later.
  uint32_t appendBlock(OpCode Code, const BlockType &Type,
                       uint32_t Offset = 0) {
    return appendImm(Code, Offset, Instruction::BlocksImm{0, 0, Type});
  }

  /// Append a try_table instruction with the catch clauses. The block
  /// parameter number and the jump distances are patched later.
  uint32_t appendTryTable(const BlockType &Type,
                          Span<const Instruction::CatchDescriptor> Catch,
                          uint32_t Offset = 0) {
    const uint32_t Pos = appendHeader(
        OpCode::Try_table, Offset,
        sizeof(Instruction::TryDescriptor) +
            static_cast<uint64_t>(sizeof(Instruction::CatchDescriptor)) *
                Catch.size());
    if (likely(Pos != InvalidPos)) {
      auto &Desc = getInstr(Pos).getTryCatch();
      Desc.ResType = Type;
      Desc.CatchNum = static_cast<uint32_t>(Catch.size());
      std::copy(Catch.begin(), Catch.end(), Desc.getCatchList().begin());
    }
    return Pos;
  }

  /// Append an instruction with an index immediate.
  uint32_t appendIndex(OpCode Code, uint32_t Target, uint32_t Offset = 0) {
    return appendImm(Code, Offset, Instruction::IndexImm{Target});
  }

  /// Append an instruction with two index immediates. The second index is
  /// also the stack offset of the local instructions patched in validation.
  uint32_t appendIndices(OpCode Code, uint32_t Target, uint32_t Source,
                         uint32_t Offset = 0) {
    return appendImm(Code, Offset, Instruction::IndicesImm{Target, Source});
  }

  /// Append a branch instruction. The jump descriptor except the label index
  /// is patched in validation.
  uint32_t appendJump(OpCode Code, uint32_t LabelIdx, uint32_t Offset = 0) {
    return appendImm(Code, Offset,
                     Instruction::JumpDescriptor{LabelIdx, 0, 0, 0});
  }

  /// Append a br_table instruction with the label indices, including the
  /// default label as the last entry.
  uint32_t appendBrTable(Span<const uint32_t> Labels, uint32_t Offset = 0) {
    const uint32_t Pos = appendHeader(
        OpCode::Br_table, Offset,
        sizeof(Instruction::ListImm) +
            static_cast<uint64_t>(sizeof(Instruction::JumpDescriptor)) *
                Labels.size());
    if (likely(Pos != InvalidPos)) {
      auto Instr = getInstr(Pos);
      Instr.getImm<Instruction::ListImm>()->Size =
          static_cast<uint32_t>(Labels.size());
      auto List = Instr.getLabelList();
      for (size_t I = 0; I < Labels.size(); ++I) {
        List[I].TargetIndex = Labels[I];
      }
    }
    return Pos;
  }

  /// Append a br_on_cast instruction.
  uint32_t appendBrCast(OpCode Code, uint32_t LabelIdx, const ValType &RType1,
                        const ValType &RType2, uint32_t Offset = 0) {
    return appendImm(
        Code, Offset,
        Instruction::BrCastDescriptor{{LabelIdx, 0, 0, 0}, RType1, RType2});
  }

  /// Append an instruction with a value type immediate.
  uint32_t appendValType(OpCode Code, const ValType &Type,
                         uint32_t Offset = 0) {
    return appendImm(Code, Offset, Type);
  }

  /// Append a select instruction with the value type list.
  uint32_t appendSelectT(Span<const ValType> Types, uint32_t Offset = 0) {
    const uint32_t Pos =
        appendHeader(OpCode::Select_t, Offset,
                     sizeof(Instruction::ListImm) +
                         static_cast<uint64_t>(sizeof(ValType)) * Types.size());
    if (likely(Pos != InvalidPos)) {
      auto Instr = getInstr(Pos);
      Instr.getImm<Instruction::ListImm>()->Size =
          static_cast<uint32_t>(Types.size());
      std::copy(Types.begin(), Types.end(), Instr.getValTypeList().begin());
    }
    return Pos;
  }

  /// Append a memory instruction.
  uint32_t appendMemory(OpCode Code, uint32_t MemIdx, uint32_t Align,
                        uint64_t MemOffset, uint32_t Offset = 0) {
    Instruction::MemoryImm Imm{MemIdx, Align, {}};
    std::memcpy(Imm.MemOffset, &MemOffset, sizeof(MemOffset));
    return appendImm(Code, Offset, Imm);
  }

  /// Append a memory instruction with a lane index.
  uint32_t appendMemoryLane(OpCode Code, uint32_t MemIdx, uint32_t Align,
                            uint64_t MemOffset, uint8_t Lane,
                            uint32_t Offset = 0) {
    Instruction::MemoryLaneImm Imm{{MemIdx, Align, {}}, Lane};
    std::memcpy(Imm.Memory.MemOffset, &MemOffset, sizeof(MemOffset));
    return appendImm(Code, Offset, Imm);
  }

  /// Append a constant instruction. The scalar constants keep the leading 8
  /// bytes of the zero-extended value.
  uint32_t appendNum(OpCode Code, const ValVariant &Val, uint32_t Offset = 0) {
    size_t ValSize = sizeof(uint128_t), ImmSize = sizeof(uint64_t);
    switch (Code) {
    case OpCode::I32__const:
    case OpCode::F32__const:
      ValSize = sizeof(uint32_t);
      break;
    case OpCode::I64__const:
    case OpCode::F64__const:
      ValSize = sizeof(uint64_t);
      break;
    default:
      ImmSize = sizeof(uint128_t);
      break;
    }
    const uint32_t Pos = appendHeader(Code, Offset, ImmSize);
    if (likely(Pos != InvalidPos)) {
      std::memcpy(getInstr(Pos).getImm<uint8_t>(),
                  static_cast<const void *>(&Val), ValSize);
    }
    return Pos;
  }

  /// Append an instruction with a lane index immediate.
  uint32_t appendLane(OpCode Code, uint8_t Lane, uint32_t Offset = 0) {
    return appendImm(Code, Offset, Instruction::LaneImm{Lane});
  }

private:
  friend class InstrView;

  /// Append a zeroed record with the header set.
  uint32_t appendHeader(OpCode Code, uint32_t Offset, uint64_t ImmSize) {
    const uint64_t Size = sizeof(Instruction::Header) + ImmSize;
    assuming(Size % alignof(Instruction::Header) == 0);
    if (unlikely(Size > MaxSize - Data.size())) {
      return InvalidPos;
    }
    const uint32_t Pos = static_cast<uint32_t>(Data.size());
    Data.resize(Data.size() + static_cast<size_t>(Size));
    *getInstr(Pos).getHeader() = {static_cast<uint16_t>(Code),
                                  Size < Instruction::OversizeMark
                                      ? static_cast<uint16_t>(Size)
                                      : Instruction::OversizeMark,
                                  Offset};
    return Pos;
  }

  /// Append a record with the immediate.
  template <typename T>
  uint32_t appendImm(OpCode Code, uint32_t Offset, const T &Imm) {
    const uint32_t Pos = appendHeader(Code, Offset, sizeof(T));
    if (likely(Pos != InvalidPos)) {
      *getInstr(Pos).getImm<T>() = Imm;
    }
    return Pos;
  }

  /// The flat instruction buffer.
  std::vector<uint8_t> Data;
};

inline InstrView::InstrView(const InstrVec &Vec) noexcept
    : Begin(Vec.Data.data()), End(Vec.Data.data() + Vec.Data.size()) {}

} // namespace AST
} // namespace WasmEdge
