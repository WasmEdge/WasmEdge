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
  LLVM::Type ExecCtxTy;
  LLVM::Type ExecCtxPtrTy;
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
  std::vector<uint32_t> Tags;
  LLVM::Value IntrinsicsTable;
  LLVM::FunctionCallee Trap;
  CompileContext(LLVM::Context C, LLVM::Module &M,
                 bool IsGenericBinary) noexcept;
  LLVM::Value getMemory(LLVM::Builder &Builder, LLVM::Value ExecCtx,
                        uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ExecCtx, 0);
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
  LLVM::Value getMemorySize(LLVM::Builder &Builder, LLVM::Value ExecCtx,
                            uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ExecCtx, 1);
    auto VPtr = Builder.createLoad(
        Int64PtrTy, Builder.createInBoundsGEP1(Int64PtrTy, Array,
                                               LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    return Builder.createLoad(Int64Ty, VPtr);
  }
  LLVM::Value getTable(LLVM::Builder &Builder, LLVM::Value ExecCtx,
                       uint32_t Index) noexcept {
    auto RefPtrTy = Int64x2Ty.getPointerTo();
    auto Array = Builder.createExtractValue(ExecCtx, 2);
    auto VPtrPtr = Builder.createLoad(
        RefPtrTy.getPointerTo(),
        Builder.createInBoundsGEP1(RefPtrTy.getPointerTo(), Array,
                                   LLContext.getInt64(Index)));
    VPtrPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                        LLVM::Metadata(LLContext, {}));
    return Builder.createLoad(
        RefPtrTy,
        Builder.createInBoundsGEP1(RefPtrTy, VPtrPtr, LLContext.getInt64(0)));
  }
  LLVM::Value getTableSize(LLVM::Builder &Builder, LLVM::Value ExecCtx,
                           uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ExecCtx, 3);
    auto VPtr = Builder.createLoad(
        Int64PtrTy, Builder.createInBoundsGEP1(Int64PtrTy, Array,
                                               LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    return Builder.createLoad(Int64Ty, VPtr);
  }
  std::pair<LLVM::Type, LLVM::Value> getGlobal(LLVM::Builder &Builder,
                                               LLVM::Value ExecCtx,
                                               uint32_t Index) noexcept {
    auto Ty = Globals[Index];
    auto Array = Builder.createExtractValue(ExecCtx, 4);
    auto VPtr = Builder.createLoad(
        Int128PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                                LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    auto Ptr = Builder.createBitCast(VPtr, Ty.getPointerTo());
    return {Ty, Ptr};
  }
  LLVM::Value getTag(LLVM::Builder &Builder, LLVM::Value ExecCtx,
                     uint32_t Index) noexcept {
    auto Array = Builder.createExtractValue(ExecCtx, 5);
    auto VPtr = Builder.createLoad(
        Int8PtrTy, Builder.createInBoundsGEP1(Int8PtrTy, Array,
                                              LLContext.getInt64(Index)));
    VPtr.setMetadata(LLContext, LLVM::Core::InvariantGroup,
                     LLVM::Metadata(LLContext, {}));
    return VPtr;
  }
  LLVM::Value getPendingExnTagAddr(LLVM::Builder &Builder,
                                   LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 6);
  }
  LLVM::Value getInstrCount(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 7);
  }
  LLVM::Value getCostTable(LLVM::Builder &Builder,
                           LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 8);
  }
  LLVM::Value getGas(LLVM::Builder &Builder, LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 9);
  }
  LLVM::Value getGasLimit(LLVM::Builder &Builder,
                          LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 10);
  }
  LLVM::Value getStopToken(LLVM::Builder &Builder,
                           LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 11);
  }
  LLVM::Value getModuleInst(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 12);
  }
  LLVM::Value getStackLimit(LLVM::Builder &Builder,
                            LLVM::Value ExecCtx) noexcept {
    return Builder.createExtractValue(ExecCtx, 13);
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
toLLVMArgsType(LLVM::Context LLContext, LLVM::Type ExecCtxPtrTy,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
LLVM::Type
toLLVMRetsType(LLVM::Context LLContext,
               WasmEdge::Span<const WasmEdge::ValType> ValTypes) noexcept;
LLVM::Type toLLVMType(LLVM::Context LLContext, LLVM::Type ExecCtxPtrTy,
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
