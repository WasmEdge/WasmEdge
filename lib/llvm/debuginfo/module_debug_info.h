// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_die.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/type_translator.h"

#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace WasmEdge::LLVM::DebugInfo {

class ModuleDebugInfoImpl final : public ModuleDebugInfo {
public:
  ModuleDebugInfoImpl(llvm::Module &M, std::unique_ptr<DwarfReader> R, Part P,
                      bool HasMemory, bool LinesOnly) noexcept;

  ~ModuleDebugInfoImpl() noexcept override { finalize(); }

  LLVMMetadataRef translateType(uint64_t DieOffset) noexcept override;

  bool linesOnly() const noexcept { return LinesOnly; }

  LLVMValueRef getMemoryBaseGlobal() const noexcept override {
    return llvm::wrap(MemoryBase);
  }

  std::unique_ptr<FunctionDebugInfo>
  beginFunction(uint32_t FuncIndex, uint32_t DefinedIndex, LLVMValueRef Fn,
                bool ExtendLiveness) noexcept override;

  void finalize() noexcept override;

  std::optional<std::string> verify() noexcept override;

  uint64_t toAddress(uint64_t FileOffset) const noexcept {
    return Reader->toAddress(FileOffset);
  }

  std::pair<llvm::DIScope *, llvm::DILocation *>
  scopeAt(UnitState &S, llvm::DISubprogram *SP, DWARFDie FnDie, uint64_t Addr,
          const ScopeIndex &Scopes) noexcept;

  /// Scope, or a DILexicalBlockFile in it when File differs from the file
  /// of Scope or the line table row has a discriminator.
  llvm::DIScope *fileScope(UnitState &S, llvm::DIScope *Scope,
                           llvm::DIFile *File,
                           unsigned Discriminator = 0) noexcept;

  llvm::DIScope *lexicalBlock(UnitState &S, llvm::DIScope *Scope,
                              DWARFDie Block) noexcept;

  llvm::DIFile *variableFile(UnitState &S, DWARFDie Var,
                             llvm::DIFile *Fallback) noexcept;

  llvm::ConstantInt *constantOf(DWARFDie Var) noexcept;

  /// The low Width bits of Value as an integer constant of that width.
  llvm::ConstantInt *integerValue(unsigned Width, uint64_t Value) noexcept;

  void addGlobal(UnitState &S, llvm::DIScope *Scope, DWARFDie D,
                 uint64_t Address) noexcept;

  TypeTranslator &types() noexcept { return Translator; }

private:
  void createMemoryBase() noexcept;

  /// Describes a global with DW_AT_const_value. It needs no memory base.
  void addConstantGlobal(UnitState &S, llvm::DIScope *Scope,
                         DWARFDie D) noexcept;

  void addUnitGlobals() noexcept;

  void addScopeGlobals(DWARFDie Parent, uint32_t Depth) noexcept;

  /// The subprogram of D, in the compile unit of D. Inlined copies in other
  /// units share it.
  llvm::DISubprogram *subprogramFor(DWARFDie D) noexcept;

  /// Creates a definition that no other function shares. LLVM requires one
  /// definition for each function, and one source subprogram can cover the
  /// bodies of several wasm functions.
  llvm::DISubprogram *createSubprogram(UnitState &S, DWARFDie D) noexcept;

  UnitState &artificialUnit() noexcept;

  llvm::DIScope *scopeFor(UnitState &S, llvm::DIScope *Parent, DWARFDie Frame,
                          uint64_t Addr, const ScopeIndex &Scopes) noexcept;

  UnitState &unit(DWARFDie Die) noexcept;

  llvm::Module &M;
  std::unique_ptr<DwarfReader> Reader;
  TypeTranslator Translator;
  std::map<uint64_t, UnitState> Units;
  std::unordered_map<uint64_t, llvm::DISubprogram *> Subprograms;
  std::unordered_set<const llvm::DISubprogram *> Attached;
  struct ScopeOffsetLess {
    bool operator()(
        const std::pair<const llvm::DIScope *, uint64_t> &L,
        const std::pair<const llvm::DIScope *, uint64_t> &R) const noexcept {
      if (L.first != R.first) {
        return std::less<const llvm::DIScope *>()(L.first, R.first);
      }
      return L.second < R.second;
    }
  };
  std::map<std::pair<const llvm::DIScope *, uint64_t>, llvm::DIScope *,
           ScopeOffsetLess>
      Lexical;
  struct ScopeFileKey {
    llvm::DIScope *Scope;
    llvm::DIFile *File;
    unsigned Discriminator;
  };
  struct ScopeFileLess {
    bool operator()(const ScopeFileKey &L,
                    const ScopeFileKey &R) const noexcept {
      if (L.Scope != R.Scope) {
        return std::less<llvm::DIScope *>()(L.Scope, R.Scope);
      }
      if (L.File != R.File) {
        return std::less<llvm::DIFile *>()(L.File, R.File);
      }
      return L.Discriminator < R.Discriminator;
    }
  };
  std::map<ScopeFileKey, llvm::DIScope *, ScopeFileLess> FileScopes;
  std::unordered_set<uint64_t> Globals;
  UnitState Artificial;
  Part P;
  bool LinesOnly;
  bool Finalized = false;
  llvm::GlobalVariable *MemoryBase = nullptr;
};

} // namespace WasmEdge::LLVM::DebugInfo
