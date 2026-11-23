// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/function_debug_info.h"
#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/module_debug_info.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#if LLVM_VERSION_MAJOR < 16
#include <llvm/ADT/Triple.h>
#else
#include <llvm/Support/ModRef.h>
#include <llvm/TargetParser/Triple.h>
#endif

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

namespace {

bool isLocalValue(const WasmLocation &L) noexcept {
  return L.K == WasmLocation::Kind::Local && !L.Indirect;
}

} // namespace

FunctionDebugInfoImpl::FunctionDebugInfoImpl(ModuleDebugInfoImpl &Parent,
                                             UnitState &S, llvm::Function &Fn,
                                             llvm::DISubprogram *SP,
                                             DWARFDie Die,
                                             bool ExtendLiveness) noexcept
    : Parent(Parent), S(S), Fn(Fn), SP(SP), Die(Die),
      ExtendLiveness(ExtendLiveness) {
  Fn.setSubprogram(SP);
  if (!Die) {
    return;
  }
  auto *U = Die.getDwarfUnit();
  LineTable = U->getContext().getLineTableForUnit(U);
  if (auto FB = Die.find(dwarf::DW_AT_frame_base); FB && !Parent.linesOnly()) {
    if (auto Block = FB->getAsBlock()) {
      const auto L =
          decodeLocation(*Block, U, WasmLocationConvention::Standard);
      if (L.K == WasmLocation::Kind::Local) {
        FrameBaseSlot = addressSlot(AddressSlot::Kind::Local, L.Index);
      } else if (L.K == WasmLocation::Kind::Global) {
        FrameBaseSlot = addressSlot(AddressSlot::Kind::Global, L.Index);
      }
    }
  }
  collectScopes(Die, Die, 0, false);
  Scopes.build();
  std::stable_sort(
      ScopeEnds.begin(), ScopeEnds.end(),
      [](const ScopeEnd &L, const ScopeEnd &R) { return L.End < R.End; });
  std::sort(ScopeBounds.begin(), ScopeBounds.end());
  ScopeBounds.erase(std::unique(ScopeBounds.begin(), ScopeBounds.end()),
                    ScopeBounds.end());
  if (Parent.linesOnly()) {
    return;
  }
  unsigned ArgNo = 0;
  collectVariables(Die, SP, ArgNo, 0);
  for (size_t I = 0; I < Variables.size(); ++I) {
    for (const auto &E : Variables[I].Ranges) {
      RangeEvents.emplace_back(E.Low, I);
      RangeEvents.emplace_back(E.High, I);
      if (!isLocalValue(E.L)) {
        continue;
      }
      auto &Users = LocalUsers[static_cast<uint32_t>(E.L.Index)];
      if (Users.empty() || Users.back() != I) {
        Users.push_back(I);
      }
    }
  }
  std::sort(RangeEvents.begin(), RangeEvents.end());
}

bool FunctionDebugInfoImpl::hasVariables() const noexcept {
  return !Parent.linesOnly();
}

size_t FunctionDebugInfoImpl::addressSlot(AddressSlot::Kind K,
                                          uint64_t Index) noexcept {
  const auto I = static_cast<uint32_t>(Index);
  auto [It, Inserted] =
      AddressSlotIndex.emplace(slotKey(K, I), AddressSlots.size());
  if (Inserted) {
    AddressSlots.push_back({K, I});
  }
  return It->second;
}

bool FunctionDebugInfoImpl::placeVariable(Variable &V,
                                          const WasmLocation &L) noexcept {
  V.L = L;
  switch (L.K) {
  case WasmLocation::Kind::Local:
    if (L.Indirect) {
      V.Slot = addressSlot(AddressSlot::Kind::Local, L.Index);
    }
    return true;
  case WasmLocation::Kind::Global:
    V.Slot = addressSlot(L.Indirect ? AddressSlot::Kind::Global
                                    : AddressSlot::Kind::GlobalStorage,
                         L.Index);
    return true;
  case WasmLocation::Kind::FrameBaseOffset:
    V.Slot = FrameBaseSlot;
    return FrameBaseSlot != NoIndex;
  default:
    return false;
  }
}

void FunctionDebugInfoImpl::collectVariables(DWARFDie Node,
                                             llvm::DIScope *Scope,
                                             unsigned &ArgNo,
                                             uint32_t Depth) noexcept {
  auto &DB = *S.DB;
  for (auto C : Node.children()) {
    const auto Tag = C.getTag();
    if (Tag == dwarf::DW_TAG_lexical_block) {
      if (Depth < MaxDepth) {
        collectVariables(C, Parent.lexicalBlock(S, Scope, C), ArgNo, Depth + 1);
      }
      continue;
    }
    if (Tag != dwarf::DW_TAG_formal_parameter &&
        Tag != dwarf::DW_TAG_variable) {
      continue;
    }
    const bool IsParam = Tag == dwarf::DW_TAG_formal_parameter;
    if (IsParam) {
      ++ArgNo;
    }
    const auto Int = integerType(ref(C));
    auto Loc = locationOf(C, Int.has_value());
    if (const auto Address = globalAddress(Loc)) {
      Parent.addGlobal(S, Scope, C, *Address);
      continue;
    }
    Variable V;
    V.ScopeOffset = Node.getOffset();
    V.Scope = Scope;
    if (Int) {
      V.ConstantBits = Int->first;
    }
    auto rangeValue = [&V](const WasmLocation &L) {
      return isLocalValue(L) ||
             (L.K == WasmLocation::Kind::Constant && V.ConstantBits != 0);
    };
    bool Available = true;
    llvm::Constant *Value = nullptr;
    if (!Loc) {
      Value = Parent.constantOf(C);
    } else if (Loc->Single && rangeValue(*Loc->Single) &&
               Loc->Single->K == WasmLocation::Kind::Constant) {
      Value = Parent.integerValue(V.ConstantBits, Loc->Single->Index);
    }
    if (Value) {
      V.Constant = Constants.size();
      Constants.push_back(Value);
    } else if (!Loc) {
      Available = false;
    } else if (Loc->Single) {
      Available = placeVariable(V, *Loc->Single);
    } else {
      const auto &First = Loc->Ranges.front().L;
      const bool Same = std::all_of(
          Loc->Ranges.begin(), Loc->Ranges.end(),
          [&](const LocationEntry &E) { return sameLocation(E.L, First); });
      if (Same && !rangeValue(First) &&
          First.K != WasmLocation::Kind::Address) {
        Available = coversScope(Node, Loc->Ranges) && placeVariable(V, First);
      } else {
        for (auto E : Loc->Ranges) {
          if (!rangeValue(E.L)) {
            E.L = WasmLocation{};
          }
          V.Ranges.push_back(E);
        }
        Available = std::any_of(V.Ranges.begin(), V.Ranges.end(),
                                [](const LocationEntry &E) {
                                  return E.L.K != WasmLocation::Kind::None;
                                });
      }
    }
    auto *Ty = Parent.types().variableType(S, C);
    const char *Name = C.getName(llvm::DINameKind::ShortName);
    const auto Line = static_cast<unsigned>(C.getDeclLine());
    auto *File = Parent.variableFile(S, C, SP->getFile());
    auto Flags = llvm::DINode::FlagZero;
    if (dwarf::toUnsigned(C.findRecursively(dwarf::DW_AT_artificial), 0)) {
      Flags |= llvm::DINode::FlagArtificial;
    }
    if (IsParam && isObjectPointer(Node, C)) {
      Flags |= llvm::DINode::FlagObjectPointer;
    }
    V.Var = IsParam ? DB.createParameterVariable(Scope, Name ? Name : "", ArgNo,
                                                 File, Line, Ty, true, Flags)
                    : DB.createAutoVariable(Scope, Name ? Name : "", File, Line,
                                            Ty, true, Flags);
    if (!Available) {
      continue;
    }
    V.Loc = llvm::DILocation::get(Fn.getContext(), Line, 0, Scope);
    Variables.push_back(std::move(V));
  }
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

void FunctionDebugInfoImpl::setLocation(LLVMBuilderRef B,
                                        uint64_t FileOffset) noexcept {
  if (!Die) {
    setArtificialLocation(B);
    return;
  }
  auto &Builder = *llvm::unwrap(B);
  const auto Addr = Parent.toAddress(FileOffset);
  recordScopeExits(Builder, Addr);
  updateRanges(Builder, Addr);
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

void FunctionDebugInfoImpl::insertValue(llvm::IRBuilderBase &B, llvm::Value *V,
                                        const Variable &Var,
                                        bool Deref) noexcept {
  auto *BB = B.GetInsertBlock();
  if (!BB) {
    return;
  }
  auto *Expr = Deref ? S.DB->createExpression(
                           llvm::SmallVector<uint64_t, 1>{dwarf::DW_OP_deref})
                     : S.DB->createExpression();
#if LLVM_VERSION_MAJOR < 20
  if (B.GetInsertPoint() == BB->end()) {
    S.DB->insertDbgValueIntrinsic(V, Var.Var, Expr, Var.Loc, BB);
  } else {
    S.DB->insertDbgValueIntrinsic(V, Var.Var, Expr, Var.Loc,
                                  &*B.GetInsertPoint());
  }
#elif LLVM_VERSION_MAJOR < 24
  S.DB->insertDbgValueIntrinsic(V, Var.Var, Expr, Var.Loc, B.GetInsertPoint());
#else
  S.DB->insertDbgValue(V, Var.Var, Expr, Var.Loc, B.GetInsertPoint());
#endif
}

void FunctionDebugInfoImpl::updateRanges(llvm::IRBuilderBase &B,
                                         uint64_t Addr) noexcept {
  if (!RangeEvents.empty()) {
    recordBlockEntry(B, Addr);
  }
  if (NextRangeEvent >= RangeEvents.size() ||
      RangeEvents[NextRangeEvent].first > Addr) {
    return;
  }
  std::vector<size_t> Changed;
  for (; NextRangeEvent < RangeEvents.size() &&
         RangeEvents[NextRangeEvent].first <= Addr;
       ++NextRangeEvent) {
    Changed.push_back(RangeEvents[NextRangeEvent].second);
  }
  std::sort(Changed.begin(), Changed.end());
  Changed.erase(std::unique(Changed.begin(), Changed.end()), Changed.end());
  for (const auto I : Changed) {
    auto &V = Variables[I];
    const auto Active = findLocationEntry(V.Ranges, Addr).value_or(NoIndex);
    if (Active == V.Active) {
      continue;
    }
    V.Active = Active;
    emitState(B, V, Active);
  }
}

void FunctionDebugInfoImpl::emitState(llvm::IRBuilderBase &B, const Variable &V,
                                      size_t Active) noexcept {
  if (Active != NoIndex &&
      V.Ranges[Active].L.K == WasmLocation::Kind::Constant) {
    insertValue(
        B, Parent.integerValue(V.ConstantBits, V.Ranges[Active].L.Index), V);
    return;
  }
  if (Active == NoIndex || !isLocalValue(V.Ranges[Active].L) ||
      V.Ranges[Active].L.Index >= Locals.size()) {
    insertValue(
        B, llvm::UndefValue::get(llvm::Type::getInt32Ty(Fn.getContext())), V);
    return;
  }
  auto *Slot = llvm::cast<llvm::AllocaInst>(Locals[V.Ranges[Active].L.Index]);
  if (LocalsInMemory) {
    insertValue(B, Slot, V, true);
  } else {
    insertValue(B, B.CreateLoad(Slot->getAllocatedType(), Slot), V);
  }
}

FunctionDebugInfoImpl::RecordedPoint
FunctionDebugInfoImpl::RecordedPoint::at(llvm::IRBuilderBase &B) noexcept {
  RecordedPoint Point;
  auto *BB = B.GetInsertBlock();
  Point.Block = BB;
  const auto Pos = B.GetInsertPoint();
  Point.HasLast = Pos != BB->begin();
  if (Point.HasLast) {
    Point.Last = &*std::prev(Pos);
  }
  return Point;
}

llvm::BasicBlock *FunctionDebugInfoImpl::RecordedPoint::block() const noexcept {
  return llvm::cast_or_null<llvm::BasicBlock>(Block);
}

llvm::Instruction *
FunctionDebugInfoImpl::RecordedPoint::resolve() const noexcept {
  auto *BB = block();
  if (!BB) {
    return nullptr;
  }
  if (HasLast) {
    auto *L = llvm::cast_or_null<llvm::Instruction>(Last);
    return L && L->getParent() == BB ? L->getNextNode() : nullptr;
  }
  const auto It = BB->getFirstInsertionPt();
  return It != BB->end() ? &*It : nullptr;
}

void FunctionDebugInfoImpl::recordBlockEntry(llvm::IRBuilderBase &B,
                                             uint64_t Addr) noexcept {
  auto *BB = B.GetInsertBlock();
  if (!BB) {
    return;
  }
  auto [It, Inserted] = BlockIndex.emplace(BB, Blocks.size());
  if (!Inserted) {
    Blocks[It->second].End = Addr;
    return;
  }
  BlockEntry Entry;
  Entry.Point = RecordedPoint::at(B);
  Entry.Start = Addr;
  Entry.End = Addr;
  Blocks.push_back(std::move(Entry));
}

void FunctionDebugInfoImpl::addEntryStates() noexcept {
  auto endOf = [this](const llvm::BasicBlock *P) -> std::optional<uint64_t> {
    for (uint32_t Depth = 0; P && Depth < MaxDepth; ++Depth) {
      if (auto It = BlockIndex.find(P); It != BlockIndex.end()) {
        return Blocks[It->second].End;
      }
      P = P->getUniquePredecessor();
    }
    return std::nullopt;
  };
  std::vector<size_t> Seen(Variables.size(), NoIndex);
  std::vector<size_t> Changed;
  for (size_t N = 0; N < Blocks.size(); ++N) {
    const auto &Entry = Blocks[N];
    auto *BB = Entry.Point.block();
    if (!BB) {
      continue;
    }
    Changed.clear();
    auto collect = [&](uint64_t Low, uint64_t High, bool WithHigh) {
      auto It = std::upper_bound(
          RangeEvents.begin(), RangeEvents.end(), Low,
          [](uint64_t A, const std::pair<uint64_t, size_t> &E) {
            return A < E.first;
          });
      for (; It != RangeEvents.end() &&
             (It->first < High || (WithHigh && It->first == High));
           ++It) {
        if (Seen[It->second] != N) {
          Seen[It->second] = N;
          Changed.push_back(It->second);
        }
      }
    };
    std::optional<uint64_t> Earliest;
    std::optional<uint64_t> Latest;
    for (auto *P : llvm::predecessors(BB)) {
      const auto End = endOf(P);
      if (!End) {
        continue;
      }
      if (*End < Entry.Start) {
        Earliest = std::min(Earliest.value_or(*End), *End);
      } else if (*End > Entry.Start) {
        Latest = std::max(Latest.value_or(*End), *End);
      }
    }
    if (Earliest) {
      collect(*Earliest, Entry.Start, false);
    }
    if (Latest) {
      collect(Entry.Start, *Latest, true);
    }
    if (Changed.empty()) {
      continue;
    }
    auto *At = Entry.Point.resolve();
    if (!At) {
      continue;
    }
    if (llvm::isa<llvm::PHINode>(At) || At->isEHPad()) {
      auto It = BB->getFirstInsertionPt();
      if (It == BB->end()) {
        continue;
      }
      At = &*It;
    }
    llvm::IRBuilder<> B(At);
    B.SetCurrentDebugLocation(llvm::DebugLoc());
    std::sort(Changed.begin(), Changed.end());
    for (const auto I : Changed) {
      const auto &V = Variables[I];
      emitState(B, V,
                findLocationEntry(V.Ranges, Entry.Start).value_or(NoIndex));
    }
  }
}

void FunctionDebugInfoImpl::localWritten(LLVMBuilderRef B, uint32_t Index,
                                         LLVMValueRef Value) noexcept {
  auto It = LocalUsers.find(Index);
  if (It == LocalUsers.end()) {
    return;
  }
  if (LocalsInMemory) {
    return;
  }
  for (const auto I : It->second) {
    const auto &V = Variables[I];
    if (V.Active != NoIndex && isLocalValue(V.Ranges[V.Active].L) &&
        V.Ranges[V.Active].L.Index == Index) {
      insertValue(*llvm::unwrap(B), llvm::unwrap(Value), V);
    }
  }
}

void FunctionDebugInfoImpl::finish(LLVMBasicBlockRef EntryRef) noexcept {
  auto &Entry = *llvm::unwrap(EntryRef);
  auto *Terminator = Entry.getTerminator();
  if (!Terminator) {
    return;
  }
  addEntryStates();
  if (ExtendLiveness) {
    keepInMemory(MemorySlot);
    for (auto *Slot : Slots) {
      keepInMemory(Slot);
    }
  }
#if LLVM_VERSION_MAJOR < 20
  const InsertPoint Where = Terminator;
#else
  const InsertPoint Where = Terminator->getIterator();
#endif
  auto &DB = *S.DB;
  if (MemorySlot) {
    auto *Ty = DB.createPointerType(
        DB.createBasicType("unsigned char", 8, dwarf::DW_ATE_unsigned_char),
        Parent.types().pointerBits());
    auto *Var = DB.createAutoVariable(SP, "__wasm_memory", SP->getFile(), 0, Ty,
                                      true, llvm::DINode::FlagArtificial);
    DB.insertDeclare(
        MemorySlot, Var, DB.createExpression(),
        llvm::DILocation::get(Fn.getContext(), SP->getLine(), 0, SP), Where);
  }
  std::vector<LocalVariable> Declared;
  llvm::IRBuilder<> AtEntry(&Entry, Entry.begin());
  for (const auto &V : Variables) {
    if (V.Constant != NoIndex) {
      insertValue(AtEntry, Constants[V.Constant], V);
      continue;
    }
    if (!V.Ranges.empty()) {
      const auto Local = std::find_if(
          V.Ranges.begin(), V.Ranges.end(),
          [](const LocationEntry &E) { return isLocalValue(E.L); });
      if (Local == V.Ranges.end()) {
        continue;
      }
      const auto Index = Local->L.Index;
      if (Index < Locals.size() && std::all_of(
                                       V.Ranges.begin(), V.Ranges.end(),
                                       [Index](const LocationEntry &E) {
                                         return !isLocalValue(E.L) ||
                                                E.L.Index == Index;
                                       })) {
        Declared.push_back(
            {V.ScopeOffset, V.Scope, static_cast<size_t>(Index)});
      }
      continue;
    }
    if (V.Slot != NoIndex) {
      if (V.Slot >= Slots.size() || !Slots[V.Slot]) {
        continue;
      }
      llvm::SmallVector<uint64_t, 4> Ops = {dwarf::DW_OP_deref};
      if (V.L.K == WasmLocation::Kind::FrameBaseOffset) {
        if (V.L.Offset >= 0) {
          Ops.append(
              {dwarf::DW_OP_plus_uconst, static_cast<uint64_t>(V.L.Offset)});
        } else {
          Ops.append({dwarf::DW_OP_constu,
                      UINT64_C(0) - static_cast<uint64_t>(V.L.Offset),
                      dwarf::DW_OP_minus});
        }
      }
      DB.insertDeclare(Slots[V.Slot], V.Var, DB.createExpression(Ops), V.Loc,
                       Where);
      continue;
    }
    if (V.L.K == WasmLocation::Kind::Local && V.L.Index < Locals.size()) {
      DB.insertDeclare(Locals[V.L.Index], V.Var, DB.createExpression(), V.Loc,
                       Where);
      Declared.push_back(
          {V.ScopeOffset, V.Scope, static_cast<size_t>(V.L.Index)});
    }
  }
  if (ExtendLiveness) {
    addFakeUses(Declared);
  }
}

void FunctionDebugInfoImpl::collectScopes(DWARFDie Node, DWARFDie Frame,
                                          uint32_t Depth,
                                          bool Inlined) noexcept {
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
        if (R.LowPC >= R.HighPC) {
          continue;
        }
        ScopeBounds.push_back(R.LowPC);
        ScopeBounds.push_back(R.HighPC);
        if (Tag == dwarf::DW_TAG_lexical_block) {
          Scopes.add(Frame, C, Depth, R.LowPC, R.HighPC);
        }
        if (ExtendLiveness && Tag == dwarf::DW_TAG_lexical_block && !Inlined) {
          ScopeEnds.push_back({R.HighPC, C.getOffset()});
          RangedScopes.insert(C.getOffset());
        }
      }
    } else {
      llvm::consumeError(Ranges.takeError());
    }
    const bool IsInlined = Tag == dwarf::DW_TAG_inlined_subroutine;
    collectScopes(C, IsInlined ? C : Frame, Depth + 1, Inlined || IsInlined);
  }
}

void FunctionDebugInfoImpl::recordScopeExits(llvm::IRBuilderBase &B,
                                             uint64_t Addr) noexcept {
  if (NextScopeEnd >= ScopeEnds.size() || ScopeEnds[NextScopeEnd].End > Addr) {
    return;
  }
  if (!B.GetInsertBlock()) {
    return;
  }
  ScopeExit Exit;
  Exit.Point = RecordedPoint::at(B);
  for (; NextScopeEnd < ScopeEnds.size() && ScopeEnds[NextScopeEnd].End <= Addr;
       ++NextScopeEnd) {
    Exit.Offsets.push_back(ScopeEnds[NextScopeEnd].Offset);
  }
  ScopeExits.push_back(std::move(Exit));
}

void FunctionDebugInfoImpl::keepInMemory(llvm::Value *Slot) noexcept {
  if (!Slot) {
    return;
  }
  for (auto *U : Slot->users()) {
    if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(U)) {
      Load->setVolatile(true);
    } else if (auto *Store = llvm::dyn_cast<llvm::StoreInst>(U);
               Store && Store->getPointerOperand() == Slot) {
      Store->setVolatile(true);
    }
  }
}

namespace {

#if LLVM_VERSION_MAJOR < 20
void emitAsmFakeUse(llvm::IRBuilder<> &B, const llvm::Triple &TT,
                    llvm::Value *V) noexcept {
  std::vector<llvm::Value *> Ops;
  std::string Constraints;
  auto Add = [&](llvm::Value *Op, const char *C) {
    if (!Constraints.empty()) {
      Constraints += ',';
    }
    Constraints += C;
    Ops.push_back(Op);
  };
  auto *Ty = V->getType();
  if (!Ty->isFloatingPointTy() && !Ty->isVectorTy()) {
    Add(V, "r");
  } else if (TT.getArch() == llvm::Triple::x86 ||
             TT.getArch() == llvm::Triple::x86_64) {
    Add(V, "x");
  } else if (TT.isAArch64()) {
    Add(V, "w");
  } else if (Ty->isVectorTy()) {
#if LLVM_VERSION_MAJOR < 11
    auto *VecTy = llvm::VectorType::get(B.getInt64Ty(), 2);
#else
    auto *VecTy = llvm::FixedVectorType::get(B.getInt64Ty(), 2);
#endif
    auto *Vec = B.CreateBitCast(V, VecTy);
    Add(B.CreateExtractElement(Vec, uint64_t(0)), "r");
    Add(B.CreateExtractElement(Vec, uint64_t(1)), "r");
  } else {
    Add(B.CreateBitCast(V, B.getIntNTy(Ty->getPrimitiveSizeInBits())), "r");
  }
  std::vector<llvm::Type *> Types;
  for (auto *Op : Ops) {
    Types.push_back(Op->getType());
  }
  auto *FTy = llvm::FunctionType::get(B.getVoidTy(), Types, false);
  auto *Call =
      B.CreateCall(FTy, llvm::InlineAsm::get(FTy, "", Constraints, true), Ops);
  Call->setDoesNotThrow();
#if LLVM_VERSION_MAJOR < 14
  Call->addAttribute(llvm::AttributeList::FunctionIndex,
                     llvm::Attribute::InaccessibleMemOnly);
#elif LLVM_VERSION_MAJOR < 16
  Call->addFnAttr(llvm::Attribute::InaccessibleMemOnly);
#else
  Call->setMemoryEffects(llvm::MemoryEffects::inaccessibleMemOnly());
#endif
  Call->setTailCallKind(llvm::CallInst::TCK_NoTail);
}
#endif

} // namespace

void FunctionDebugInfoImpl::addFakeUses(
    const std::vector<LocalVariable> &Declared) noexcept {
  if (Declared.empty()) {
    return;
  }
  std::vector<size_t> FunctionScope;
  std::unordered_map<uint64_t, std::vector<size_t>> ScopeLocals;
  std::unordered_map<const llvm::DIScope *, uint64_t> ScopeOffsets;
  for (const auto &V : Declared) {
    if (RangedScopes.count(V.ScopeOffset)) {
      ScopeLocals[V.ScopeOffset].push_back(V.Index);
      ScopeOffsets.emplace(V.Scope, V.ScopeOffset);
    } else {
      FunctionScope.push_back(V.Index);
    }
  }

  std::vector<std::pair<llvm::Instruction *, std::vector<size_t>>> Points;
  std::unordered_map<llvm::Instruction *, size_t> PointIndex;
  auto Want = [&](llvm::Instruction *At) -> std::vector<size_t> & {
    if (auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(At)) {
      if (auto *Call =
              llvm::dyn_cast_or_null<llvm::CallInst>(Ret->getPrevNode());
          Call && Call->isTailCall()) {
        At = Call;
      }
    }
    auto [It, Inserted] = PointIndex.emplace(At, Points.size());
    if (Inserted) {
      Points.emplace_back(At, std::vector<size_t>());
    }
    return Points[It->second].second;
  };
  auto AddScope = [&](std::vector<size_t> &Set, uint64_t Offset) {
    if (auto It = ScopeLocals.find(Offset); It != ScopeLocals.end()) {
      Set.insert(Set.end(), It->second.begin(), It->second.end());
    }
  };

  for (const auto &Exit : ScopeExits) {
    auto *At = Exit.Point.resolve();
    if (!At || llvm::isa<llvm::PHINode>(At) || At->isEHPad()) {
      continue;
    }
    auto &Set = Want(At);
    for (auto Offset : Exit.Offsets) {
      AddScope(Set, Offset);
    }
  }
  for (auto &BB : Fn) {
    auto *Ret = llvm::dyn_cast_or_null<llvm::ReturnInst>(BB.getTerminator());
    if (!Ret) {
      continue;
    }
    auto &Set = Want(Ret);
    Set.insert(Set.end(), FunctionScope.begin(), FunctionScope.end());
    const auto *Loc = Ret->getDebugLoc().get();
    for (const llvm::DIScope *Scope = Loc ? Loc->getInlinedAtScope() : nullptr;
         Scope && Scope != SP; Scope = Scope->getScope()) {
      if (auto It = ScopeOffsets.find(Scope); It != ScopeOffsets.end()) {
        AddScope(Set, It->second);
      }
    }
  }

#if LLVM_VERSION_MAJOR < 20
  const llvm::Triple TT(Fn.getParent()->getTargetTriple());
#else
  llvm::Function *FakeUse = nullptr;
#endif
  for (auto &[At, Set] : Points) {
    std::sort(Set.begin(), Set.end());
    Set.erase(std::unique(Set.begin(), Set.end()), Set.end());
    llvm::IRBuilder<> B(At);
    B.SetCurrentDebugLocation(llvm::DebugLoc());
    for (const auto I : Set) {
      auto *Slot = llvm::dyn_cast<llvm::AllocaInst>(Locals[I]);
      if (!Slot) {
        continue;
      }
      auto *Value = B.CreateLoad(Slot->getAllocatedType(), Slot, "fake.use");
#if LLVM_VERSION_MAJOR < 20
      emitAsmFakeUse(B, TT, Value);
#else
      if (!FakeUse) {
        FakeUse = llvm::Intrinsic::getOrInsertDeclaration(
            Fn.getParent(), llvm::Intrinsic::fake_use);
      }
      auto *Call = B.CreateCall(FakeUse, {Value});
      Call->setDoesNotThrow();
      Call->setTailCallKind(llvm::CallInst::TCK_NoTail);
#endif
    }
  }
}

} // namespace WasmEdge::LLVM::DebugInfo
