// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/function_debug_info.h"
#include "debuginfo/module_debug_info.h"

#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>

#include <algorithm>
#include <cstdint>

namespace WasmEdge::LLVM::DebugInfo {

void FunctionDebugInfoImpl::setLocation(LLVMBuilderRef B,
                                        uint64_t FileOffset) noexcept {
  if (!Die) {
    setArtificialLocation(B);
    return;
  }
  auto &Builder = *llvm::unwrap(B);
  const auto Addr = Parent.toAddress(FileOffset);
  if (Addr < CachedLow || Addr >= CachedHigh) {
    std::tie(CachedScope, CachedInlinedAt) =
        Parent.scopeAt(S, SP, Die, Addr, Scopes);
    const auto It =
        std::upper_bound(ScopeBounds.begin(), ScopeBounds.end(), Addr);
    CachedHigh = It == ScopeBounds.end() ? UINT64_MAX : *It;
    CachedLow = It == ScopeBounds.begin() ? 0 : *std::prev(It);
  }
  unsigned Line = 0;
  unsigned Column = 0;
  llvm::DIScope *Scope = CachedScope;
  if (LineTable) {
    const auto Row = LineTable->lookupAddress(
        {Addr, llvm::object::SectionedAddress::UndefSection});
    if (Row != UINT32_MAX) {
      const auto &R = LineTable->Rows[Row];
      Line = R.Line;
      Column = R.Column;
      Scope = Parent.fileScope(S, Scope, lineFile(R.File), R.Discriminator);
    }
  }
  Builder.SetCurrentDebugLocation(llvm::DILocation::get(
      Fn.getContext(), Line, Column, Scope, CachedInlinedAt));
}

llvm::DIFile *FunctionDebugInfoImpl::lineFile(uint64_t FileIndex) noexcept {
  if (auto It = LineFiles.find(FileIndex); It != LineFiles.end()) {
    return It->second;
  }
  const auto Path = lineTableFile(LineTable, Die.getDwarfUnit(), FileIndex);
  auto *File = Path.empty() ? nullptr : fileFor(S, Path, "");
  LineFiles.emplace(FileIndex, File);
  return File;
}

} // namespace WasmEdge::LLVM::DebugInfo
