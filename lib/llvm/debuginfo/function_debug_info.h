// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_die.h"
#include "debuginfo/type_translator.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

class ModuleDebugInfoImpl;

class FunctionDebugInfoImpl final : public FunctionDebugInfo {
public:
  FunctionDebugInfoImpl(ModuleDebugInfoImpl &Parent, UnitState &S,
                        llvm::Function &Fn, llvm::DISubprogram *SP,
                        DWARFDie Die) noexcept
      : Parent(Parent), S(S), Fn(Fn), SP(SP), Die(Die) {
    Fn.setSubprogram(SP);
    if (!Die) {
      return;
    }
    auto *U = Die.getDwarfUnit();
    LineTable = U->getContext().getLineTableForUnit(U);
    collectScopes(Die, Die, 0);
    Scopes.build();
    std::sort(ScopeBounds.begin(), ScopeBounds.end());
    ScopeBounds.erase(std::unique(ScopeBounds.begin(), ScopeBounds.end()),
                      ScopeBounds.end());
  }

  void setArtificialLocation(LLVMBuilderRef B) noexcept override {
    llvm::unwrap(B)->SetCurrentDebugLocation(
        llvm::DILocation::get(Fn.getContext(), 0, 0, SP));
  }

  void setLocation(LLVMBuilderRef B, uint64_t FileOffset) noexcept override;

  FrameBase getFrameBase() const noexcept override { return {}; }

  void finish(LLVMBasicBlockRef, Span<const LLVMValueRef>, LLVMValueRef,
              LLVMValueRef, LLVMValueRef) noexcept override {}

private:
  void collectScopes(DWARFDie Node, DWARFDie Frame, uint32_t Depth) noexcept {
    if (Depth >= MaxDepth) {
      return;
    }
    for (auto C : Node.children()) {
      const auto Tag = C.getTag();
      if (Tag != dwarf::DW_TAG_lexical_block &&
          Tag != dwarf::DW_TAG_inlined_subroutine) {
        continue;
      }
      if (auto Ranges = C.getAddressRanges()) {
        for (const auto &R : *Ranges) {
          if (R.LowPC < R.HighPC) {
            ScopeBounds.push_back(R.LowPC);
            ScopeBounds.push_back(R.HighPC);
            if (Tag == dwarf::DW_TAG_lexical_block) {
              Scopes.add(Frame, C, Depth, R.LowPC, R.HighPC);
            }
          }
        }
      } else {
        llvm::consumeError(Ranges.takeError());
      }
      collectScopes(C, Tag == dwarf::DW_TAG_inlined_subroutine ? C : Frame,
                    Depth + 1);
    }
  }

  llvm::DIFile *lineFile(uint64_t FileIndex) noexcept;

  ModuleDebugInfoImpl &Parent;
  UnitState &S;
  llvm::Function &Fn;
  llvm::DISubprogram *SP;
  DWARFDie Die;
  const llvm::DWARFDebugLine::LineTable *LineTable = nullptr;
  std::vector<uint64_t> ScopeBounds;
  ScopeIndex Scopes;
  uint64_t CachedLow = 1;
  uint64_t CachedHigh = 0;
  llvm::DIScope *CachedScope = nullptr;
  llvm::DILocation *CachedInlinedAt = nullptr;
  std::unordered_map<uint64_t, llvm::DIFile *> LineFiles;
};

} // namespace WasmEdge::LLVM::DebugInfo
