// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/dwarf_die.h"

#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/LLVMContext.h>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

struct UnitState {
  std::unique_ptr<llvm::DIBuilder> DB;
  llvm::DICompileUnit *CU = nullptr;
};

llvm::DIFile *fileFor(UnitState &S, llvm::StringRef Path,
                      llvm::StringRef Dir) noexcept;

llvm::DIFile *file(UnitState &S, DWARFDie D) noexcept;

class TypeTranslator {
public:
  explicit TypeTranslator(llvm::LLVMContext &Ctx) noexcept : Ctx(Ctx) {}

  llvm::DIType *type(UnitState &S, DWARFDie D) noexcept;

  /// The scope of a DIE: its namespace, its struct, class, or union, or the
  /// compile unit.
  llvm::DIScope *typeScope(UnitState &S, DWARFDie D) noexcept;

private:
  llvm::DIType *uchar(UnitState &S) noexcept;

  llvm::DIType *uint32(UnitState &S) noexcept;

  llvm::DIType *opaque(UnitState &S, uint64_t Size) noexcept;

  llvm::DIType *funcRef(UnitState &S) noexcept;

  llvm::DIType *wasmPointer(UnitState &S, DWARFDie D) noexcept;

  std::string pointerName(DWARFDie Pointee);

  /// Gives null for a member whose offset cannot be read, so that no member
  /// shows at a wrong place.
  llvm::DIDerivedType *member(UnitState &S, llvm::DIScope *Scope,
                              DWARFDie C) noexcept;

  /// Gives null if one variant cannot be represented. Leaving out only that
  /// variant would let a default variant match its values.
  llvm::Metadata *variantPart(UnitState &S, llvm::DIScope *Scope,
                              DWARFDie VP) noexcept;

  llvm::DIType *composite(UnitState &S, DWARFDie D) noexcept;

  llvm::DIType *enumeration(UnitState &S, DWARFDie D) noexcept;

  llvm::DIType *array(UnitState &S, DWARFDie D) noexcept;

  llvm::DISubroutineType *subroutine(UnitState &S, DWARFDie D) noexcept;

  llvm::LLVMContext &Ctx;
  std::unordered_map<uint64_t, llvm::DIType *> Types;
  std::unordered_map<std::string, uint64_t> PointerNames;
  /// The synthetic pointer types of each compile unit, by the key of their
  /// pointee. Pointers to the same pointee share one layout and one name.
  static constexpr uint64_t VoidPointeeKey = UINT64_MAX;
  static constexpr uint64_t FuncRefKey = UINT64_MAX - 1;
  std::map<std::pair<uintptr_t, uint64_t>, llvm::DIType *> PointerTypes;
};

} // namespace WasmEdge::LLVM::DebugInfo
