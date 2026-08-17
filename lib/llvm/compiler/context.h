// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "llvm/compiler.h"

#include "llvm.h"

#include "aot/version.h"
#include "system/allocator.h"

#include <cstdint>
#include <functional>

#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge::LLVM {

// XXX: Misalignment handler not implemented yet, forcing unalignment
// force unalignment load/store
inline constexpr const bool kForceUnalignment = true;

// force checking div/rem on zero
inline constexpr const bool kForceDivCheck = true;

// Size of a ValVariant
inline constexpr const uint32_t kValSize = sizeof(WasmEdge::ValVariant);

struct Compiler::CompileContext {
  LLVM::Context LLContext;
  std::reference_wrapper<LLVM::Module> LLModule;
  LLVM::Attribute Cold;
  LLVM::Attribute NoAlias;
  LLVM::Attribute NoInline;
  LLVM::Attribute NoReturn;
  LLVM::Attribute ReadOnly;
  LLVM::Attribute StrictFP;
  LLVM::Attribute UWTable;
  LLVM::Attribute NoStackArgProbe;
  LLVM::Type VoidTy;
  LLVM::Type Int8Ty;
  LLVM::Type Int16Ty;
  LLVM::Type Int32Ty;
  LLVM::Type Int64Ty;
  LLVM::Type Int128Ty;
  LLVM::Type FloatTy;
  LLVM::Type DoubleTy;
  LLVM::Type Int8x16Ty;
  LLVM::Type Int16x8Ty;
  LLVM::Type Int32x4Ty;
  LLVM::Type Floatx4Ty;
  LLVM::Type Int64x2Ty;
  LLVM::Type Doublex2Ty;
  LLVM::Type Int128x1Ty;
  LLVM::Type Int8PtrTy;
  LLVM::Type Int32PtrTy;
  LLVM::Type Int64PtrTy;
  LLVM::Type Int128PtrTy;
  LLVM::Type Int8PtrPtrTy;
  LLVM::Type ModCtxTy;
  LLVM::Type ModCtxPtrTy;
  LLVM::Type ExecCtxTy;
  LLVM::Type ExecCtxPtrTy;
  // GC shadow-frame node {ShadowFrame* Prev; uint32_t Count; ValVariant* Slots}
  // -- layout kept in lockstep with GC::Controller::ShadowFrame. Generated code
  // pushes one of these onto the thread's shadow-head chain around calls.
  LLVM::Type ShadowFrameTy;
  LLVM::Type ShadowFramePtrTy;
  LLVM::Type IntrinsicsTableTy;
  LLVM::Type IntrinsicsTablePtrTy;
  LLVM::Message SubtargetFeatures;

#if defined(__x86_64__)
#if defined(__XOP__)
  bool SupportXOP = true;
#else
  bool SupportXOP = false;
#endif

#if defined(__SSE4_1__)
  bool SupportSSE4_1 = true;
#else
  bool SupportSSE4_1 = false;
#endif

#if defined(__SSSE3__)
  bool SupportSSSE3 = true;
#else
  bool SupportSSSE3 = false;
#endif

#if defined(__SSE2__)
  bool SupportSSE2 = true;
#else
  bool SupportSSE2 = false;
#endif
#endif

#if defined(__aarch64__)
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(__ARM_NEON_FP)
  bool SupportNEON = true;
#else
  bool SupportNEON = false;
#endif
#endif

  std::vector<const AST::CompositeType *> CompositeTypes;
  std::vector<LLVM::Value> FunctionWrappers;
  std::vector<std::tuple<uint32_t, LLVM::FunctionCallee,
                         const WasmEdge::AST::CodeSegment *>>
      Functions;
  std::vector<LLVM::Value> LazyJITCacheVars;
  uint32_t ImportCount = 0;
  std::vector<LLVM::Type> MemoryAddrTypes;
  std::vector<LLVM::Type> TableAddrTypes;
  std::vector<LLVM::Type> Globals;
  // Parallel to Globals: whether each global is reference-typed. A ref-typed
  // global.set must also run the GC write barrier (kWriteBarrier) on the old
  // and new reference; a numeric global stores directly with no barrier.
  std::vector<bool> GlobalIsRef;
  std::vector<uint32_t> Tags;
  LLVM::Value IntrinsicsTable;
  LLVM::FunctionCallee Trap;
  // Whether the GC proposal is enabled for this compilation. Drives emission of
  // the GC-specific codegen that a concurrent collector relies on -- the
  // cooperative safepoint poll (checkGCSafepoint) and the shadow-root spill
  // around calls. When false, this artifact is NOT GC-capable: it never yields
  // at a safepoint and never publishes native roots, so it must not run
  // natively under a GC-enabled executor (the instantiate gate falls it back to
  // the interpreter). Coherent ref global/table access stays UNCONDITIONAL
  // (externrefs cross into non-GC modules), so it is not gated on this flag.
  bool GCEnabled = false;
  CompileContext(LLVM::Context C, LLVM::Module &M, bool IsGenericBinary,
                 bool GCEnabled) noexcept;
  LLVM::Value getMemory(LLVM::Builder &Builder, LLVM::Value ModCtx,
                        uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ModCtx, 0);
#if WASMEDGE_ALLOCATOR_IS_STABLE
    auto VPtr = Builder.createLoad(
        Int8PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                              LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
#else
    auto VPtrPtr = Builder.createLoad(
        Int8PtrPtrTy, Builder.createInBoundsGEP1(Int8PtrPtrTy, Array,
                                                 LLContext.getInt64(Index)));
    VPtrPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                        LLVM::Metadata(LLContext, {}));
    auto VPtr = Builder.createLoad(
        Int8PtrTy,
        Builder.createInBoundsGEP1(Int8PtrTy, VPtrPtr, LLContext.getInt64(0)));
#endif
    return Builder.createBitCast(VPtr, Int8PtrTy);
  }
  LLVM::Value getMemorySize(LLVM::Builder &Builder, LLVM::Value ModCtx,
                            uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ModCtx, 1);
    auto VPtr = Builder.createLoad(
        Int64PtrTy, Builder.createInBoundsGEP1(Int64PtrTy, Array,
                                               LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    return Builder.createLoad(Int64Ty, VPtr);
  }
  LLVM::Value getTable(LLVM::Builder &Builder, LLVM::Value ModCtx,
                       uint32_t Index) noexcept {
    auto RefPtrTy = Int64x2Ty.getPointerTo();
    auto Array = Builder.createExtractValue(ModCtx, 2);
    auto VPtrPtr = Builder.createLoad(
        RefPtrTy.getPointerTo(),
        Builder.createInBoundsGEP1(RefPtrTy.getPointerTo(), Array,
                                   LLContext.getInt64(Index)));
    VPtrPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                        LLVM::Metadata(LLContext, {}));
    // Acquire load of the table's base pointer: pairs with growTable's release
    // store, so a reader that observes the new base also sees the new buffer's
    // contents (no torn/stale read) and does not data-race that store.
    auto DataPtrVal = Builder.createLoad(
        RefPtrTy,
        Builder.createInBoundsGEP1(RefPtrTy, VPtrPtr, LLContext.getInt64(0)));
    DataPtrVal.setOrdering(LLVMAtomicOrderingAcquire);
    DataPtrVal.setAlignment(8);
    return DataPtrVal;
  }
  LLVM::Value getTableSize(LLVM::Builder &Builder, LLVM::Value ModCtx,
                           uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ModCtx, 3);
    auto VPtr = Builder.createLoad(
        Int64PtrTy, Builder.createInBoundsGEP1(Int64PtrTy, Array,
                                               LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    // Acquire load of the table's live size: pairs with growTable's release
    // store of LiveSize (published AFTER the base pointer), so observing the
    // new size implies the new base is visible -- the bounds check and buffer
    // access stay consistent (no OOB) and do not data-race the store.
    auto SizeVal = Builder.createLoad(Int64Ty, VPtr);
    SizeVal.setOrdering(LLVMAtomicOrderingAcquire);
    SizeVal.setAlignment(8);
    return SizeVal;
  }
  LLVM::Value getModuleInst(LLVM::Builder &Builder,
                            LLVM::Value ModCtx) noexcept {
    return Builder.createExtractValue(ModCtx, 5);
  }
  std::pair<LLVM::Type, LLVM::Value> getGlobal(LLVM::Builder &Builder,
                                               LLVM::Value ModCtx,
                                               uint32_t Index) noexcept {
    auto Ty = Globals[Index];
    auto Array = Builder.createExtractValue(ModCtx, 4);
    auto VPtr = Builder.createLoad(
        Int128PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                                LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    auto Ptr = Builder.createBitCast(VPtr, Ty.getPointerTo());
    return {Ty, Ptr};
  }
  LLVM::Value getTag(LLVM::Builder &Builder, LLVM::Value ModCtx,
                     uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ModCtx, 6);
    auto VPtr = Builder.createLoad(
        Int8PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                              LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    return VPtr;
  }
  LLVM::Value getPendingExnTagAddr(LLVM::Builder &Builder,
                                   LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 5);
  }
  LLVM::Value getInstrCount(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 0);
  }
  LLVM::Value getCostTable(LLVM::Builder &Builder,
                           LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 1);
  }
  LLVM::Value getGas(LLVM::Builder &Builder, LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 2);
  }
  LLVM::Value getGasLimit(LLVM::Builder &Builder,
                          LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 3);
  }
  LLVM::Value getStopToken(LLVM::Builder &Builder,
                           LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 4);
  }
  // GC::Controller::ShadowHead* (ExecCtx index 6, after PendingExnTagAddr).
  // Yields the raw pointer to this thread's stable shadow-root chain anchor,
  // into which generated code publishes spilled managed refs for the collector
  // to scan. Thread state, not module state, so it lives in ExecCtx and stays
  // fixed across a cross-module call that swaps ModCtx.
  LLVM::Value getShadowHead(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 6);
  }
  // std::atomic<bool>* the GC controller's stop flag (ExecCtx index 7).
  // Generated code loads a byte from it at loop back-edges; a set flag routes
  // to kGCSafepoint.
  LLVM::Value getGCStopFlag(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 7);
  }
  LLVM::FunctionCallee getIntrinsic(LLVM::Builder &Builder,
                                    Executable::Intrinsics Index,
                                    LLVM::Type Ty) noexcept {
    const auto Value = static_cast<uint32_t>(Index);
    auto PtrTy = Ty.getPointerTo();
    auto PtrPtrTy = PtrTy.getPointerTo();
    auto IT = Builder.createLoad(IntrinsicsTablePtrTy, IntrinsicsTable);
    IT.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                   LLVM::Metadata(LLContext, {}));
    auto VPtr =
        Builder.createInBoundsGEP2(IntrinsicsTableTy, IT, LLContext.getInt64(0),
                                   LLContext.getInt64(Value));
    auto Ptr = Builder.createBitCast(VPtr, PtrPtrTy);
    return {Ty, Builder.createLoad(PtrTy, Ptr)};
  }
  void compileTrap() noexcept {
    LLVM::Builder Builder(LLContext);
    Builder.positionAtEnd(
        LLVM::BasicBlock::create(LLContext, Trap.Fn, "entry"));
    auto FnTy = LLVM::Type::getFunctionType(VoidTy, {Int32Ty});
    auto CallTrap = Builder.createCall(
        getIntrinsic(Builder, Executable::Intrinsics::kTrap, FnTy),
        {Trap.Fn.getFirstParam()});
    CallTrap.addCallSiteAttribute(NoReturn);
    Builder.createUnreachable();
  }
  void addVersionGlobal() noexcept {
    LLModule.get().addGlobal(
        Int32Ty, true, LLVMExternalLinkage,
        LLVM::Value::getConstInt(Int32Ty, AOT::kBinaryVersion), "version");
  }
  /// Emit an exported marker global recording that this artifact was compiled
  /// with GC codegen (safepoint polls + shadow-root spill). It is the single
  /// source of truth for the capability once the artifact leaves this process:
  /// a native shared library is probed for the symbol directly, and a universal
  /// WASM records its presence as a flag byte in the AOT section. Emitted only
  /// when GC codegen is on, so absence means "not GC-capable" -- unambiguous
  /// because artifacts predating this marker are already rejected by the
  /// kBinaryVersion check.
  void addGCCapableGlobal() noexcept {
    if (!GCEnabled) {
      return;
    }
    LLModule.get().addGlobal(Int32Ty, true, LLVMExternalLinkage,
                             LLVM::Value::getConstInt(Int32Ty, UINT32_C(1)),
                             "gc.capable");
  }
  void finalizeIntrinsicsTable() noexcept {
    if (auto Table = LLModule.get().getNamedGlobal("intrinsics")) {
      Table.setInitializer(LLVM::Value::getConstNull(Table.getType()));
      Table.setGlobalConstant(false);
    } else {
      LLModule.get().addGlobal(IntrinsicsTablePtrTy, false, LLVMExternalLinkage,
                               LLVM::Value::getConstNull(IntrinsicsTablePtrTy),
                               "intrinsics");
    }
  }
  std::pair<std::vector<ValType>, std::vector<ValType>>
  resolveBlockType(const BlockType &BType) const noexcept {
    using VecT = std::vector<ValType>;
    using RetT = std::pair<VecT, VecT>;
    if (BType.isEmpty()) {
      return RetT{};
    }
    if (BType.isValType()) {
      return RetT{{}, {BType.getValType()}};
    } else {
      // Type index case. t2* = type[index].returns
      const uint32_t TypeIdx = BType.getTypeIndex();
      const auto &FType = CompositeTypes[TypeIdx]->getFuncType();
      return RetT{
          VecT(FType.getParamTypes().begin(), FType.getParamTypes().end()),
          VecT(FType.getReturnTypes().begin(), FType.getReturnTypes().end())};
    }
  }
};

bool isVoidReturn(WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
LLVM::Type toLLVMType(LLVM::Context LLContext,
                      const WasmEdge::ValType &ValType) noexcept;
std::vector<LLVM::Type>
toLLVMArgsType(LLVM::Context LLContext, LLVM::Type ModCtxPtrTy,
               LLVM::Type ExecCtxPtrTy,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
LLVM::Type
toLLVMRetsType(LLVM::Context LLContext,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
LLVM::Type toLLVMType(LLVM::Context LLContext, LLVM::Type ModCtxPtrTy,
                      LLVM::Type ExecCtxPtrTy,
                      const WasmEdge::AST::FunctionType &FuncType) noexcept;
LLVM::Value
toLLVMConstantZero(LLVM::Context LLContext, const WasmEdge::ValType &ValType,
                   WasmEdge::Span<const WasmEdge::AST::CompositeType *const>
                       CompositeTypes) noexcept;
std::vector<LLVM::Value> unpackStruct(LLVM::Builder &Builder,
                                      LLVM::Value Struct) noexcept;
LLVM::Type toLLVMType(LLVM::Context LLContext,
                      const WasmEdge::AddressType AddrType) noexcept;
std::vector<LLVM::Type>
toLLVMTypeVector(LLVM::Context LLContext,
                 WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;

} // namespace WasmEdge::LLVM
