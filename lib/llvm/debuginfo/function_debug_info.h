// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_die.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/type_translator.h"

#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

class ModuleDebugInfoImpl;

class FunctionDebugInfoImpl final : public FunctionDebugInfo {
public:
  FunctionDebugInfoImpl(ModuleDebugInfoImpl &Parent, UnitState &S,
                        llvm::Function &Fn, llvm::DISubprogram *SP,
                        DWARFDie Die) noexcept;

  void setArtificialLocation(LLVMBuilderRef B) noexcept override {
    llvm::unwrap(B)->SetCurrentDebugLocation(
        llvm::DILocation::get(Fn.getContext(), 0, 0, SP));
  }

  void setLocation(LLVMBuilderRef B, uint64_t FileOffset) noexcept override;

  Span<const AddressSlot> getAddressSlots() const noexcept override {
    return AddressSlots;
  }

  std::optional<size_t>
  findAddressSlot(AddressSlot::Kind K, uint32_t Index) const noexcept override {
    if (auto It = AddressSlotIndex.find(slotKey(K, Index));
        It != AddressSlotIndex.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  bool hasVariables() const noexcept override;

  void bindFrame(Span<const LLVMValueRef> LocalRefs, LLVMValueRef Memory,
                 Span<const LLVMValueRef> SlotRefs,
                 bool InMemory) noexcept override {
#if LLVM_VERSION_MAJOR < 18
    static_cast<void>(InMemory);
    LocalsInMemory = false;
#else
    LocalsInMemory = InMemory;
#endif
    Locals.clear();
    for (auto *L : LocalRefs) {
      Locals.push_back(llvm::unwrap(L));
    }
    MemorySlot = llvm::unwrap(Memory);
    Slots.clear();
    for (auto *Slot : SlotRefs) {
      Slots.push_back(llvm::unwrap(Slot));
    }
  }

  void localWritten(LLVMBuilderRef B, uint32_t Index,
                    LLVMValueRef Value) noexcept override;

  void finish(LLVMBasicBlockRef EntryRef) noexcept override;

private:
#if LLVM_VERSION_MAJOR < 20
  using InsertPoint = llvm::Instruction *;
#else
  using InsertPoint = llvm::BasicBlock::iterator;
#endif
  static constexpr size_t NoIndex = SIZE_MAX;

  static uint64_t slotKey(AddressSlot::Kind K, uint32_t Index) noexcept {
    return (static_cast<uint64_t>(K) << 32) | Index;
  }

  struct Variable {
    llvm::DILocalVariable *Var = nullptr;
    llvm::DILocation *Loc = nullptr;
    WasmLocation L;
    size_t Slot = NoIndex;
    std::vector<LocationEntry> Ranges;
    size_t Active = NoIndex;
    size_t Constant = NoIndex;
    unsigned ConstantBits = 0;
  };

  /// A place in a native block that the compiler passed. Later passes can
  /// delete the block or the instruction before the place.
  struct RecordedPoint {
    llvm::WeakVH Block;
    llvm::WeakVH Last;
    bool HasLast = false;

    static RecordedPoint at(llvm::IRBuilderBase &B) noexcept;
    llvm::BasicBlock *block() const noexcept;
    /// The instruction that is now at the place, or null if the place is
    /// gone.
    llvm::Instruction *resolve() const noexcept;
  };

  struct BlockEntry {
    RecordedPoint Point;
    uint64_t Start = 0;
    uint64_t End = 0;
  };

  size_t addressSlot(AddressSlot::Kind K, uint64_t Index) noexcept;
  bool placeVariable(Variable &V, const WasmLocation &L) noexcept;
  void collectVariables(DWARFDie Node, llvm::DIScope *Scope, unsigned &ArgNo,
                        uint32_t Depth) noexcept;
  void collectScopes(DWARFDie Node, DWARFDie Frame, uint32_t Depth) noexcept;
  void updateRanges(llvm::IRBuilderBase &B, uint64_t Addr) noexcept;
  void emitState(llvm::IRBuilderBase &B, const Variable &V,
                 size_t Active) noexcept;
  void recordBlockEntry(llvm::IRBuilderBase &B, uint64_t Addr) noexcept;
  /// The native blocks do not follow the address order of the wasm code. A
  /// block gets the location that a variable has at its start address when
  /// the location changes between the end of a predecessor and that start.
  void addEntryStates() noexcept;
  void insertValue(llvm::IRBuilderBase &B, llvm::Value *V, const Variable &Var,
                   bool Deref = false) noexcept;
  llvm::DIFile *lineFile(uint64_t FileIndex) noexcept;

  ModuleDebugInfoImpl &Parent;
  UnitState &S;
  llvm::Function &Fn;
  llvm::DISubprogram *SP;
  DWARFDie Die;
  const llvm::DWARFDebugLine::LineTable *LineTable = nullptr;
  size_t FrameBaseSlot = NoIndex;
  std::vector<AddressSlot> AddressSlots;
  std::unordered_map<uint64_t, size_t> AddressSlotIndex;
  std::vector<llvm::Value *> Locals;
  llvm::Value *MemorySlot = nullptr;
  std::vector<llvm::Value *> Slots;
  std::vector<llvm::Constant *> Constants;
  bool LocalsInMemory = false;
  std::vector<Variable> Variables;
  std::vector<std::pair<uint64_t, size_t>> RangeEvents;
  size_t NextRangeEvent = 0;
  std::vector<BlockEntry> Blocks;
  std::unordered_map<const llvm::BasicBlock *, size_t> BlockIndex;
  std::unordered_map<uint32_t, std::vector<size_t>> LocalUsers;
  std::vector<uint64_t> ScopeBounds;
  ScopeIndex Scopes;
  uint64_t CachedLow = 1;
  uint64_t CachedHigh = 0;
  llvm::DIScope *CachedScope = nullptr;
  llvm::DILocation *CachedInlinedAt = nullptr;
  std::unordered_map<uint64_t, llvm::DIFile *> LineFiles;
};

} // namespace WasmEdge::LLVM::DebugInfo
