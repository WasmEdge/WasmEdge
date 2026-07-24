// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/context.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge::LLVM {

Compiler::CompileContext::CompileContext(LLVM::Context C, LLVM::Module &M,
                                         bool IsGenericBinary) noexcept
    : LLContext(C), LLModule(M),
      Cold(LLVM::Attribute::createEnum(C, LLVM::Core::Cold, 0)),
      NoAlias(LLVM::Attribute::createEnum(C, LLVM::Core::NoAlias, 0)),
      NoInline(LLVM::Attribute::createEnum(C, LLVM::Core::NoInline, 0)),
      NoReturn(LLVM::Attribute::createEnum(C, LLVM::Core::NoReturn, 0)),
      ReadOnly(LLVM::Attribute::createEnum(C, LLVM::Core::ReadOnly, 0)),
      StrictFP(LLVM::Attribute::createEnum(C, LLVM::Core::StrictFP, 0)),
      UWTable(LLVM::Attribute::createEnum(C, LLVM::Core::UWTable,
                                          LLVM::Core::UWTableDefault)),
      NoStackArgProbe(
          LLVM::Attribute::createString(C, "no-stack-arg-probe"sv, {})),
      VoidTy(LLContext.getVoidTy()), Int8Ty(LLContext.getInt8Ty()),
      Int16Ty(LLContext.getInt16Ty()), Int32Ty(LLContext.getInt32Ty()),
      Int64Ty(LLContext.getInt64Ty()), Int128Ty(LLContext.getInt128Ty()),
      FloatTy(LLContext.getFloatTy()), DoubleTy(LLContext.getDoubleTy()),
      Int8x16Ty(LLVM::Type::getVectorType(Int8Ty, 16)),
      Int16x8Ty(LLVM::Type::getVectorType(Int16Ty, 8)),
      Int32x4Ty(LLVM::Type::getVectorType(Int32Ty, 4)),
      Floatx4Ty(LLVM::Type::getVectorType(FloatTy, 4)),
      Int64x2Ty(LLVM::Type::getVectorType(Int64Ty, 2)),
      Doublex2Ty(LLVM::Type::getVectorType(DoubleTy, 2)),
      Int128x1Ty(LLVM::Type::getVectorType(Int128Ty, 1)),
      Int8PtrTy(Int8Ty.getPointerTo()), Int32PtrTy(Int32Ty.getPointerTo()),
      Int64PtrTy(Int64Ty.getPointerTo()), Int128PtrTy(Int128Ty.getPointerTo()),
      Int8PtrPtrTy(Int8PtrTy.getPointerTo()),
      ExecCtxTy(LLVM::Type::getStructType(
          "ExecCtx",
          std::initializer_list<LLVM::Type>{
              // MemoryPtrs
              Int8PtrTy.getPointerTo(),
              // MemorySizes
              Int64PtrTy.getPointerTo(),
              // TableRefs
              Int64x2Ty.getPointerTo().getPointerTo(),
              // TableSizes
              Int64PtrTy.getPointerTo(),
              // Globals
              Int128PtrTy.getPointerTo(),
              // Tags
              Int8PtrPtrTy,
              // PendingExnTagAddr
              Int8PtrPtrTy,
              // InstrCount
              Int64PtrTy,
              // CostTable
              LLVM::Type::getArrayType(Int64Ty, UINT16_MAX + 1).getPointerTo(),
              // Gas
              Int64PtrTy,
              // GasLimit
              Int64Ty,
              // StopToken
              Int32PtrTy,
              // ModuleInst
              Int8PtrTy,
          })),
      ExecCtxPtrTy(ExecCtxTy.getPointerTo()),
      IntrinsicsTableTy(LLVM::Type::getArrayType(
          Int8Ty.getPointerTo(),
          static_cast<uint32_t>(Executable::Intrinsics::kIntrinsicMax))),
      IntrinsicsTablePtrTy(IntrinsicsTableTy.getPointerTo()),
      IntrinsicsTable(LLModule.get().addGlobal(IntrinsicsTablePtrTy, true,
                                               LLVMExternalLinkage,
                                               LLVM::Value(), "intrinsics")) {
  Trap.Ty = LLVM::Type::getFunctionType(VoidTy, {Int32Ty});
  Trap.Fn = LLModule.get().addFunction(Trap.Ty, LLVMPrivateLinkage, "trap");
  Trap.Fn.setDSOLocal(true);
  Trap.Fn.addFnAttr(NoStackArgProbe);
  Trap.Fn.addFnAttr(StrictFP);
  Trap.Fn.addFnAttr(UWTable);
  Trap.Fn.addFnAttr(NoReturn);
  Trap.Fn.addFnAttr(Cold);
  Trap.Fn.addFnAttr(NoInline);

  if (!IsGenericBinary) {
    SubtargetFeatures = LLVM::getHostCPUFeatures();
    auto Features = SubtargetFeatures.string_view();
    while (!Features.empty()) {
      std::string_view Feature;
      if (auto Pos = Features.find(','); Pos != std::string_view::npos) {
        Feature = Features.substr(0, Pos);
        Features = Features.substr(Pos + 1);
      } else {
        Feature = std::exchange(Features, std::string_view());
      }
      if (Feature[0] != '+') {
        continue;
      }
      Feature = Feature.substr(1);

#if defined(__x86_64__)
      if (!SupportXOP && Feature == "xop"sv) {
        SupportXOP = true;
      }
      if (!SupportSSE4_1 && Feature == "sse4.1"sv) {
        SupportSSE4_1 = true;
      }
      if (!SupportSSSE3 && Feature == "ssse3"sv) {
        SupportSSSE3 = true;
      }
      if (!SupportSSE2 && Feature == "sse2"sv) {
        SupportSSE2 = true;
      }
#elif defined(__aarch64__)
      if (!SupportNEON && Feature == "neon"sv) {
        SupportNEON = true;
      }
#endif
    }
  }

  compileTrap();
}

bool isVoidReturn(Span<const ValType> ValTypes) noexcept {
  return ValTypes.empty();
}

LLVM::Type toLLVMType(LLVM::Context LLContext,
                      const ValType &ValType) noexcept {
  switch (ValType.getCode()) {
  case TypeCode::I32:
    return LLContext.getInt32Ty();
  case TypeCode::I64:
    return LLContext.getInt64Ty();
  case TypeCode::Ref:
  case TypeCode::RefNull:
  case TypeCode::V128:
    return LLVM::Type::getVectorType(LLContext.getInt64Ty(), 2);
  case TypeCode::F32:
    return LLContext.getFloatTy();
  case TypeCode::F64:
    return LLContext.getDoubleTy();
  default:
    assumingUnreachable();
  }
}

LLVM::Type toLLVMType(LLVM::Context LLContext,
                      const AddressType AddrType) noexcept {
  switch (AddrType) {
  case AddressType::I32:
    return LLContext.getInt32Ty();
  case AddressType::I64:
    return LLContext.getInt64Ty();
  default:
    assumingUnreachable();
  }
}

std::vector<LLVM::Type>
toLLVMTypeVector(LLVM::Context LLContext,
                 Span<const ValType> ValTypes) noexcept {
  std::vector<LLVM::Type> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(LLContext, Type));
  }
  return Result;
}

std::vector<LLVM::Type> toLLVMArgsType(LLVM::Context LLContext,
                                       LLVM::Type ExecCtxPtrTy,
                                       Span<const ValType> ValTypes) noexcept {
  auto Result = toLLVMTypeVector(LLContext, ValTypes);
  Result.insert(Result.begin(), ExecCtxPtrTy);
  return Result;
}

LLVM::Type toLLVMRetsType(LLVM::Context LLContext,
                          Span<const ValType> ValTypes) noexcept {
  if (isVoidReturn(ValTypes)) {
    return LLContext.getVoidTy();
  }
  if (ValTypes.size() == 1) {
    return toLLVMType(LLContext, ValTypes.front());
  }
  std::vector<LLVM::Type> Result;
  Result.reserve(ValTypes.size());
  for (const auto &Type : ValTypes) {
    Result.push_back(toLLVMType(LLContext, Type));
  }
  return LLVM::Type::getStructType(Result);
}

LLVM::Type toLLVMType(LLVM::Context LLContext, LLVM::Type ExecCtxPtrTy,
                      const AST::FunctionType &FuncType) noexcept {
  auto ArgsTy =
      toLLVMArgsType(LLContext, ExecCtxPtrTy, FuncType.getParamTypes());
  auto RetTy = toLLVMRetsType(LLContext, FuncType.getReturnTypes());
  return LLVM::Type::getFunctionType(RetTy, ArgsTy);
}

LLVM::Value toLLVMConstantZero(
    LLVM::Context LLContext, const ValType &ValType,
    Span<const AST::CompositeType *const> CompositeTypes) noexcept {
  switch (ValType.getCode()) {
  case TypeCode::I32:
    return LLVM::Value::getConstNull(LLContext.getInt32Ty());
  case TypeCode::I64:
    return LLVM::Value::getConstNull(LLContext.getInt64Ty());
  case TypeCode::Ref:
  case TypeCode::RefNull: {
    std::array<uint8_t, 16> Data{};
    if (ValType.isAbsHeapType()) {
      // Abstract heap types are already fine for null refs.
      const auto Raw = ValType.getRawData();
      std::copy(Raw.begin(), Raw.end(), Data.begin());
    } else {
      // For non-abstract heap types (concrete type indices), convert to the
      // abstract heap type so that ref.cast/ref.test won't dereference a null
      // pointer when checking the type.
      assuming(ValType.getTypeIndex() < CompositeTypes.size());
      const auto *CompType = CompositeTypes[ValType.getTypeIndex()];
      assuming(CompType != nullptr);
      WasmEdge::ValType VType =
          CompType->isFunc() ? TypeCode::NullFuncRef : TypeCode::NullRef;
      std::copy_n(VType.getRawData().cbegin(), 8, Data.begin());
    }
    return LLVM::Value::getConstVector8(LLContext, Data);
  }
  case TypeCode::V128:
    return LLVM::Value::getConstNull(
        LLVM::Type::getVectorType(LLContext.getInt64Ty(), 2));
  case TypeCode::F32:
    return LLVM::Value::getConstNull(LLContext.getFloatTy());
  case TypeCode::F64:
    return LLVM::Value::getConstNull(LLContext.getDoubleTy());
  default:
    assumingUnreachable();
  }
}

std::vector<LLVM::Value> unpackStruct(LLVM::Builder &Builder,
                                      LLVM::Value Struct) noexcept {
  const auto N = Struct.getType().getStructNumElements();
  std::vector<LLVM::Value> Ret;
  Ret.reserve(N);
  for (unsigned I = 0; I < N; ++I) {
    Ret.push_back(Builder.createExtractValue(Struct, I));
  }
  return Ret;
}

} // namespace WasmEdge::LLVM
