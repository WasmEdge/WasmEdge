// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/errcode.h"
#include "common/span.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Object.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Types.h>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge::AOT::LLVM {

class Core {
public:
  static inline void init() noexcept { std::call_once(Once, initOnce); }

  static inline const unsigned int NotIntrinsic = 0;
  static inline unsigned int Ceil = 0;
  static inline unsigned int CopySign = 0;
  static inline unsigned int Ctlz = 0;
  static inline unsigned int Cttz = 0;
  static inline unsigned int Ctpop = 0;
  static inline unsigned int Expect = 0;
  static inline unsigned int ExperimentalConstrainedFAdd = 0;
  static inline unsigned int ExperimentalConstrainedFDiv = 0;
  static inline unsigned int ExperimentalConstrainedFMul = 0;
  static inline unsigned int ExperimentalConstrainedFPExt = 0;
  static inline unsigned int ExperimentalConstrainedFPTrunc = 0;
  static inline unsigned int ExperimentalConstrainedFSub = 0;
  static inline unsigned int Fabs = 0;
  static inline unsigned int Floor = 0;
  static inline unsigned int FShl = 0;
  static inline unsigned int FShr = 0;
  static inline unsigned int MaxNum = 0;
  static inline unsigned int MinNum = 0;
  static inline unsigned int Nearbyint = 0;
  static inline unsigned int Roundeven = 0;
  static inline unsigned int SAddSat = 0;
  static inline unsigned int Sqrt = 0;
  static inline unsigned int SSubSat = 0;
  static inline unsigned int Trunc = 0;
  static inline unsigned int UAddSat = 0;
  static inline unsigned int USubSat = 0;
#if defined(__x86_64__)
  static inline unsigned int X86SSE2PAvgB = 0;
  static inline unsigned int X86SSE2PAvgW = 0;
  static inline unsigned int X86SSE2PMAddWd = 0;
  static inline unsigned int X86SSE41RoundPd = 0;
  static inline unsigned int X86SSE41RoundPs = 0;
  static inline unsigned int X86SSE41RoundSd = 0;
  static inline unsigned int X86SSE41RoundSs = 0;
  static inline unsigned int X86SSSE3PMAddUbSw128 = 0;
  static inline unsigned int X86SSSE3PMulHrSw128 = 0;
  static inline unsigned int X86SSSE3PShufB128 = 0;
  static inline unsigned int X86XOpVPHAddBW = 0;
  static inline unsigned int X86XOpVPHAddUBW = 0;
  static inline unsigned int X86XOpVPHAddUWD = 0;
  static inline unsigned int X86XOpVPHAddWD = 0;
#endif
#if defined(__aarch64__)
  static inline unsigned int AArch64NeonFRIntN = 0;
  static inline unsigned int AArch64NeonSAddLP = 0;
  static inline unsigned int AArch64NeonSQRDMulH = 0;
  static inline unsigned int AArch64NeonTbl1 = 0;
  static inline unsigned int AArch64NeonUAddLP = 0;
  static inline unsigned int AArch64NeonURHAdd = 0;
#endif

  static inline unsigned int Cold = 0;
  static inline unsigned int NoAlias = 0;
  static inline unsigned int NoInline = 0;
  static inline unsigned int NoReturn = 0;
  static inline unsigned int ReadOnly = 0;
  static inline unsigned int StrictFP = 0;

  static inline unsigned int InvariantLoad = 0;

private:
  static inline std::once_flag Once;
  static inline void initOnce() noexcept {
    using namespace std::literals;
    LLVMInitializeCore(LLVMGetGlobalPassRegistry());
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    Ceil = getIntrinsicID("llvm.ceil"sv);
    CopySign = getIntrinsicID("llvm.copysign"sv);
    Ctlz = getIntrinsicID("llvm.ctlz"sv);
    Cttz = getIntrinsicID("llvm.cttz"sv);
    Ctpop = getIntrinsicID("llvm.ctpop"sv);
    Expect = getIntrinsicID("llvm.expect");
    ExperimentalConstrainedFAdd =
        getIntrinsicID("llvm.experimental.constrained.fadd"sv);
    ExperimentalConstrainedFDiv =
        getIntrinsicID("llvm.experimental.constrained.fdiv"sv);
    ExperimentalConstrainedFMul =
        getIntrinsicID("llvm.experimental.constrained.fmul"sv);
    ExperimentalConstrainedFPExt =
        getIntrinsicID("llvm.experimental.constrained.fpext"sv);
    ExperimentalConstrainedFPTrunc =
        getIntrinsicID("llvm.experimental.constrained.fptrunc"sv);
    ExperimentalConstrainedFSub =
        getIntrinsicID("llvm.experimental.constrained.fsub"sv);
    Fabs = getIntrinsicID("llvm.fabs"sv);
    Floor = getIntrinsicID("llvm.floor"sv);
    FShl = getIntrinsicID("llvm.fshl"sv);
    FShr = getIntrinsicID("llvm.fshr"sv);
    MaxNum = getIntrinsicID("llvm.maxnum"sv);
    MinNum = getIntrinsicID("llvm.minnum"sv);
    Nearbyint = getIntrinsicID("llvm.nearbyint"sv);
    Roundeven = getIntrinsicID("llvm.roundeven"sv);
    SAddSat = getIntrinsicID("llvm.sadd.sat"sv);
    Sqrt = getIntrinsicID("llvm.sqrt"sv);
    SSubSat = getIntrinsicID("llvm.ssub.sat"sv);
    Trunc = getIntrinsicID("llvm.trunc"sv);
    UAddSat = getIntrinsicID("llvm.uadd.sat"sv);
    USubSat = getIntrinsicID("llvm.usub.sat"sv);

#if defined(__x86_64__)
    X86SSE2PAvgB = getIntrinsicID("llvm.x86.sse2.pavg.b"sv);
    X86SSE2PAvgW = getIntrinsicID("llvm.x86.sse2.pavg.w"sv);
    X86SSE2PMAddWd = getIntrinsicID("llvm.x86.sse2.pmadd.wd"sv);
    X86SSE41RoundPd = getIntrinsicID("llvm.x86.sse41.round.pd"sv);
    X86SSE41RoundPs = getIntrinsicID("llvm.x86.sse41.round.ps"sv);
    X86SSE41RoundSd = getIntrinsicID("llvm.x86.sse41.round.sd"sv);
    X86SSE41RoundSs = getIntrinsicID("llvm.x86.sse41.round.ss"sv);
    X86SSSE3PMAddUbSw128 = getIntrinsicID("llvm.x86.ssse3.pmadd.ub.sw.128"sv);
    X86SSSE3PMulHrSw128 = getIntrinsicID("llvm.x86.ssse3.pmul.hr.sw.128"sv);
    X86SSSE3PShufB128 = getIntrinsicID("llvm.x86.ssse3.pshuf.b.128"sv);
    X86XOpVPHAddBW = getIntrinsicID("llvm.x86.xop.vphaddbw"sv);
    X86XOpVPHAddUBW = getIntrinsicID("llvm.x86.xop.vphaddubw"sv);
    X86XOpVPHAddUWD = getIntrinsicID("llvm.x86.xop.vphadduwd"sv);
    X86XOpVPHAddWD = getIntrinsicID("llvm.x86.xop.vphaddwd"sv);
#endif
#if defined(__aarch64__)
    AArch64NeonFRIntN = getIntrinsicID("llvm.aarch64.neon.frintn"sv);
    AArch64NeonSAddLP = getIntrinsicID("llvm.aarch64.neon.saddlp"sv);
    AArch64NeonSQRDMulH = getIntrinsicID("llvm.aarch64.neon.sqrdmulh"sv);
    AArch64NeonTbl1 = getIntrinsicID("llvm.aarch64.neon.tbl1"sv);
    AArch64NeonUAddLP = getIntrinsicID("llvm.aarch64.neon.uaddlp"sv);
    AArch64NeonURHAdd = getIntrinsicID("llvm.aarch64.neon.urhadd"sv);
#endif

    Cold = getEnumAttributeKind("cold"sv);
    NoAlias = getEnumAttributeKind("noalias"sv);
    NoInline = getEnumAttributeKind("noinline"sv);
    NoReturn = getEnumAttributeKind("noreturn"sv);
    ReadOnly = getEnumAttributeKind("readonly"sv);
    StrictFP = getEnumAttributeKind("strictfp"sv);

    InvariantLoad = getMetadataKind("invariant.load");
  }

  template <typename... ArgsT>
  static unsigned int getIntrinsicID(std::string_view Name,
                                     ArgsT &&...Args) noexcept {
    const auto Value = LLVMLookupIntrinsicID(Name.data(), Name.size());
    if constexpr (sizeof...(Args) == 0) {
      return Value;
    } else {
      if (Value == NotIntrinsic) {
        return getIntrinsicID(std::forward<ArgsT>(Args)...);
      }
    }
  }
  static unsigned int getEnumAttributeKind(std::string_view Name) noexcept {
    return LLVMGetEnumAttributeKindForName(Name.data(), Name.size());
  }
  static unsigned int getMetadataKind(std::string_view Name) noexcept {
    return LLVMGetMDKindID(Name.data(), static_cast<unsigned int>(Name.size()));
  }
};

class Attribute;
class Message;
class Metadata;
class PassManager;
class Type;
class Value;
class Builder;
class BasicBlock;

class Context {
public:
  constexpr Context(LLVMContextRef R) noexcept : Ref(R) {}
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  Context(Context &&C) noexcept : Context() { swap(*this, C); }
  Context &operator=(Context &&C) noexcept {
    swap(*this, C);
    return *this;
  }

  Context() noexcept : Ref(LLVMContextCreate()) {}
  ~Context() noexcept { LLVMContextDispose(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(Context &LHS, Context &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  inline Type getVoidTy() noexcept;
  inline Type getInt1Ty() noexcept;
  inline Type getInt8Ty() noexcept;
  inline Type getInt16Ty() noexcept;
  inline Type getInt32Ty() noexcept;
  inline Type getInt64Ty() noexcept;
  inline Type getInt128Ty() noexcept;
  inline Type getIntNTy(unsigned int NumBits) noexcept;
  inline Type getFloatTy() noexcept;
  inline Type getDoubleTy() noexcept;

  inline Value getFalse() noexcept;
  inline Value getTrue() noexcept;
  inline Value getInt8(uint8_t C) noexcept;
  inline Value getInt16(uint16_t C) noexcept;
  inline Value getInt32(uint32_t C) noexcept;
  inline Value getInt64(uint64_t C) noexcept;
  inline Value getFloat(float C) noexcept;
  inline Value getDouble(double C) noexcept;

private:
  LLVMContextRef Ref = nullptr;
};

class Module {
public:
  constexpr Module() noexcept = default;
  constexpr Module(LLVMModuleRef R) noexcept : Ref(R) {}
  Module(const Module &) = delete;
  Module &operator=(const Module &) = delete;
  Module(Module &&M) noexcept : Module() { swap(*this, M); }
  Module &operator=(Module &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  Module(Context &C, const char *Name) noexcept
      : Ref(LLVMModuleCreateWithNameInContext(Name, C.unwrap())) {}
  ~Module() noexcept { LLVMDisposeModule(Ref); }

  const char *getTarget() noexcept { return LLVMGetTarget(Ref); }
  void setTarget(const char *Triple) noexcept { LLVMSetTarget(Ref, Triple); }
  inline Value addFunction(Type Ty, LLVMLinkage Linkage,
                           const char *Name = "") noexcept;
  inline Value addGlobal(Type Ty, bool IsConstant, LLVMLinkage Linkage,
                         Value Initializer, const char *Name = "") noexcept;
  inline Value getNamedGlobal(const char *Name) noexcept;
  inline Value addAlias(Type Ty, Value V, const char *Name,
                        unsigned int AddrSpace = 0) noexcept;
  inline void addFlag(LLVMModuleFlagBehavior Behavior, std::string_view Key,
                      const Metadata &Val) noexcept;
  inline void addFlag(LLVMModuleFlagBehavior Behavior, std::string_view Key,
                      const Value &Val) noexcept;
  inline void addFlag(LLVMModuleFlagBehavior Behavior, std::string_view Key,
                      uint32_t Val) noexcept;
  inline Value getFirstGlobal() noexcept;
  inline Value getFirstFunction() noexcept;
  inline Message printModuleToFile(const char *File) noexcept;
  inline Message verify(LLVMVerifierFailureAction Action) noexcept;

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(Module &LHS, Module &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

private:
  LLVMModuleRef Ref = nullptr;
};

class Message {
public:
  constexpr Message() noexcept = default;
  constexpr Message(char *M) noexcept : Data(M) {}
  Message(const Message &) = delete;
  Message &operator=(const Message &) = delete;
  Message(Message &&M) noexcept : Message() { swap(*this, M); }
  Message &operator=(Message &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  ~Message() noexcept { LLVMDisposeMessage(Data); }

  constexpr operator bool() const noexcept { return Data != nullptr; }
  constexpr auto &unwrap() const noexcept { return Data; }
  constexpr auto &unwrap() noexcept { return Data; }
  friend void swap(Message &LHS, Message &RHS) noexcept {
    using std::swap;
    swap(LHS.Data, RHS.Data);
  }

  std::string_view string_view() const noexcept { return Data; }

private:
  char *Data = nullptr;
};

class MemoryBuffer {
public:
  constexpr MemoryBuffer() noexcept = default;
  constexpr MemoryBuffer(LLVMMemoryBufferRef R) noexcept : Ref(R) {}
  MemoryBuffer(const MemoryBuffer &) = delete;
  MemoryBuffer &operator=(const MemoryBuffer &) = delete;
  MemoryBuffer(MemoryBuffer &&M) noexcept : MemoryBuffer() { swap(*this, M); }
  MemoryBuffer &operator=(MemoryBuffer &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  ~MemoryBuffer() noexcept { LLVMDisposeMemoryBuffer(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(MemoryBuffer &LHS, MemoryBuffer &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static std::pair<MemoryBuffer, Message> getFile(const char *Path) noexcept {
    std::pair<MemoryBuffer, Message> Result;
    LLVMCreateMemoryBufferWithContentsOfFile(Path, &Result.first.unwrap(),
                                             &Result.second.unwrap());
    return Result;
  }
  const char *data() const noexcept { return LLVMGetBufferStart(Ref); }
  size_t size() const noexcept { return LLVMGetBufferSize(Ref); }

private:
  LLVMMemoryBufferRef Ref = nullptr;
};

class Type {
public:
  constexpr Type() = default;
  constexpr Type(LLVMTypeRef R) noexcept : Ref(R) {}
  constexpr Type(const Type &) = default;
  constexpr Type &operator=(const Type &) = default;
  Type(Type &&T) noexcept : Type() { swap(*this, T); }
  Type &operator=(Type &&T) noexcept {
    swap(*this, T);
    return *this;
  }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(Type &LHS, Type &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static Type getFunctionType(Type ReturnType, Span<const Type> ParamTypes,
                              bool IsVarArg = false) noexcept {
    const auto Data = const_cast<LLVMTypeRef *>(
        reinterpret_cast<const LLVMTypeRef *>(ParamTypes.data()));
    const auto Size = static_cast<unsigned int>(ParamTypes.size());
    return LLVMFunctionType(ReturnType.unwrap(), Data, Size, IsVarArg);
  }
  static Type getPointerType(Type ElementType,
                             unsigned int AddressSpace = 0) noexcept {
    return LLVMPointerType(ElementType.unwrap(), AddressSpace);
  }
  static Type getArrayType(Type ElementType,
                           unsigned int ElementCount) noexcept {
    return LLVMArrayType(ElementType.unwrap(), ElementCount);
  }
  static Type getVectorType(Type ElementType,
                            unsigned int ElementCount) noexcept {
    return LLVMVectorType(ElementType.unwrap(), ElementCount);
  }
  static Type getStructType(Span<const Type> ElementTypes,
                            bool Packed = false) noexcept {
    const auto Data = const_cast<LLVMTypeRef *>(
        reinterpret_cast<const LLVMTypeRef *>(ElementTypes.data()));
    const auto Size = static_cast<unsigned int>(ElementTypes.size());
    assuming(Size >= 1);
    return LLVMStructTypeInContext(LLVMGetTypeContext(Data[0]), Data, Size,
                                   Packed);
  }
  static Type getStructType(const char *Name, Span<const Type> ElementTypes,
                            bool Packed = false) noexcept {
    const auto Data = const_cast<LLVMTypeRef *>(
        reinterpret_cast<const LLVMTypeRef *>(ElementTypes.data()));
    const auto Size = static_cast<unsigned int>(ElementTypes.size());
    assuming(Size >= 1);
    Type Ty = LLVMStructCreateNamed(LLVMGetTypeContext(Data[0]), Name);
    LLVMStructSetBody(Ty.unwrap(), Data, Size, Packed);
    return Ty;
  }

  bool isVoidTy() const noexcept {
    return LLVMGetTypeKind(Ref) == LLVMVoidTypeKind;
  }
  bool isFloatTy() const noexcept {
    return LLVMGetTypeKind(Ref) == LLVMFloatTypeKind;
  }
  bool isDoubleTy() const noexcept {
    return LLVMGetTypeKind(Ref) == LLVMDoubleTypeKind;
  }
  bool isIntegerTy() const noexcept {
    return LLVMGetTypeKind(Ref) == LLVMIntegerTypeKind;
  }
  bool isStructTy() const noexcept {
    return LLVMGetTypeKind(Ref) == LLVMStructTypeKind;
  }
  bool isVectorTy() const noexcept {
    return LLVMGetTypeKind(Ref) == LLVMVectorTypeKind;
  }

  unsigned int getPrimitiveSizeInBits() const noexcept {
    switch (LLVMGetTypeKind(Ref)) {
    case LLVMBFloatTypeKind:
      return 16;
    case LLVMHalfTypeKind:
      return 16;
    case LLVMFloatTypeKind:
      return 32;
    case LLVMDoubleTypeKind:
      return 64;
    case LLVMX86_FP80TypeKind:
      return 80;
    case LLVMFP128TypeKind:
      return 128;
    case LLVMPPC_FP128TypeKind:
      return 128;
    case LLVMX86_MMXTypeKind:
      return 64;
    case LLVMIntegerTypeKind:
      return getIntegerBitWidth();
    case LLVMVectorTypeKind:
      return getElementType().getPrimitiveSizeInBits() * getVectorSize();
    default:
      return 0;
    }
  }
  unsigned int getFPMantissaWidth() const noexcept {
    switch (LLVMGetTypeKind(Ref)) {
    case LLVMBFloatTypeKind:
      return 8;
    case LLVMHalfTypeKind:
      return 11;
    case LLVMFloatTypeKind:
      return 24;
    case LLVMDoubleTypeKind:
      return 53;
    case LLVMX86_FP80TypeKind:
      return 64;
    case LLVMFP128TypeKind:
      return 113;
    default:
      return 0;
    }
  }

  Type getElementType() const noexcept { return LLVMGetElementType(Ref); }
  unsigned int getIntegerBitWidth() const noexcept {
    return LLVMGetIntTypeWidth(Ref);
  }
  Type getIntegerExtendedType() const noexcept {
    return LLVMIntTypeInContext(LLVMGetTypeContext(Ref),
                                getIntegerBitWidth() * 2);
  }
  Type getIntegerTruncatedType() const noexcept {
    return LLVMIntTypeInContext(LLVMGetTypeContext(Ref),
                                getIntegerBitWidth() / 2);
  }
  unsigned int getStructNumElements() const noexcept {
    return LLVMCountStructElementTypes(Ref);
  }
  Type getStructElementType(unsigned int Index) const noexcept {
    return LLVMStructGetTypeAtIndex(Ref, Index);
  }
  Type getPointerTo(unsigned int AddressSpace = 0) const noexcept {
    return LLVMPointerType(Ref, AddressSpace);
  }
  Type getReturnType() const noexcept { return LLVMGetReturnType(Ref); }
  unsigned int getNumParams() const noexcept {
    return LLVMCountParamTypes(Ref);
  }
  void getParamTypes(Span<Type> Types) const noexcept {
    LLVMGetParamTypes(Ref, reinterpret_cast<LLVMTypeRef *>(Types.data()));
  }
  unsigned int getVectorSize() const noexcept { return LLVMGetVectorSize(Ref); }
  Type getExtendedElementVectorType() const noexcept {
    return getVectorType(getElementType().getIntegerExtendedType(),
                         getVectorSize());
  }
  Type getTruncatedElementVectorType() const noexcept {
    return getVectorType(getElementType().getIntegerTruncatedType(),
                         getVectorSize());
  }
  Type getHalfElementsVectorType() const noexcept {
    return getVectorType(getElementType(), getVectorSize() / 2);
  }

private:
  LLVMTypeRef Ref = nullptr;
};

class Value {
public:
  constexpr Value() = default;
  constexpr Value(LLVMValueRef R) noexcept : Ref(R) {}
  constexpr Value(const Value &) = default;
  constexpr Value &operator=(const Value &) = default;
  Value(Value &&V) noexcept : Value() { swap(*this, V); }
  Value &operator=(Value &&V) noexcept {
    swap(*this, V);
    return *this;
  }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }

  friend void swap(Value &LHS, Value &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static Value getConstNull(Type Ty) noexcept {
    return LLVMConstNull(Ty.unwrap());
  }
  static Value getConstAllOnes(Type Ty) noexcept {
    return LLVMConstAllOnes(Ty.unwrap());
  }
  static Value getUndef(Type Ty) noexcept { return LLVMGetUndef(Ty.unwrap()); }
  static Value getConstPointerNull(Type Ty) noexcept {
    return LLVMConstPointerNull(Ty.unwrap());
  }
  static Value getConstInt(Type IntTy, unsigned long long N,
                           bool SignExtend = false) noexcept {
    return LLVMConstInt(IntTy.unwrap(), N, SignExtend);
  }
  template <typename T> static Value getConstReal(Type Ty, T N) noexcept {
    if (Ty.isFloatTy()) {
      const auto V = static_cast<float>(N);
      uint32_t Raw;
      static_assert(sizeof(V) == sizeof(Raw));
      std::memcpy(&Raw, &V, sizeof(V));
      Type Int32Ty = LLVMInt32TypeInContext(LLVMGetTypeContext(Ty.unwrap()));
      Value Ret = getConstInt(Int32Ty, Raw);
      return LLVMConstBitCast(Ret.unwrap(), Ty.unwrap());
    }
    if (Ty.isDoubleTy()) {
      const auto V = static_cast<double>(N);
      uint64_t Raw;
      static_assert(sizeof(V) == sizeof(Raw));
      std::memcpy(&Raw, &V, sizeof(V));
      Type Int64Ty = LLVMInt64TypeInContext(LLVMGetTypeContext(Ty.unwrap()));
      Value Ret = getConstInt(Int64Ty, Raw);
      return LLVMConstBitCast(Ret.unwrap(), Ty.unwrap());
    }
    assumingUnreachable();
  }
  static Value getConstString(Context &C, std::string_view Str,
                              bool DontNullTerminate) noexcept {
    return LLVMConstStringInContext(C.unwrap(), Str.data(),
                                    static_cast<unsigned int>(Str.size()),
                                    DontNullTerminate);
  }
  static Value getConstInBoundsGEP(Type Ty, Value ConstantVal,
                                   Span<const Value> ConstantIndices) noexcept {
    const auto Data = const_cast<LLVMValueRef *>(
        reinterpret_cast<const LLVMValueRef *>(ConstantIndices.data()));
    const auto Size = static_cast<unsigned int>(ConstantIndices.size());
    return LLVMConstInBoundsGEP2(Ty.unwrap(), ConstantVal.unwrap(), Data, Size);
  }
  static Value getConstInBoundsGEP1_64(Type Ty, Value ConstantVal,
                                       uint64_t Idx0) noexcept {
    return getConstInBoundsGEP(
        Ty, ConstantVal,
        {LLVM::Value::getConstInt(
            LLVMInt64TypeInContext(LLVMGetTypeContext(Ty.unwrap())), Idx0)});
  }
  static Value getConstInBoundsGEP2_64(Type Ty, Value ConstantVal,
                                       uint64_t Idx0, uint64_t Idx1) noexcept {
    return getConstInBoundsGEP(
        Ty, ConstantVal,
        {LLVM::Value::getConstInt(
            LLVMInt64TypeInContext(LLVMGetTypeContext(Ty.unwrap())), Idx0,
            Idx1)});
  }
  static Value getConstVector(Span<const Value> ScalarConstantVals) noexcept {
    const auto Data = const_cast<LLVMValueRef *>(
        reinterpret_cast<const LLVMValueRef *>(ScalarConstantVals.data()));
    const auto Size = static_cast<unsigned int>(ScalarConstantVals.size());
    return LLVMConstVector(Data, Size);
  }
  static Value getConstVector8(Context &C,
                               Span<const uint8_t> Elements) noexcept {
    std::vector<LLVMValueRef> Data(Elements.size());
    std::transform(
        Elements.begin(), Elements.end(), Data.begin(),
        [&C](const uint8_t Element) { return C.getInt8(Element).unwrap(); });
    return LLVMConstVector(Data.data(), static_cast<unsigned int>(Data.size()));
  }
  static Value getConstVector16(Context &C,
                                Span<const uint16_t> Elements) noexcept {
    std::vector<LLVMValueRef> Data(Elements.size());
    std::transform(
        Elements.begin(), Elements.end(), Data.begin(),
        [&C](const uint16_t Element) { return C.getInt16(Element).unwrap(); });
    return LLVMConstVector(Data.data(), static_cast<unsigned int>(Data.size()));
  }
  static Value getConstVector32(Context &C,
                                Span<const uint32_t> Elements) noexcept {
    std::vector<LLVMValueRef> Data(Elements.size());
    std::transform(
        Elements.begin(), Elements.end(), Data.begin(),
        [&C](const uint32_t Element) { return C.getInt32(Element).unwrap(); });
    return LLVMConstVector(Data.data(), static_cast<unsigned int>(Data.size()));
  }
  static Value getConstVector64(Context &C,
                                Span<const uint64_t> Elements) noexcept {
    std::vector<LLVMValueRef> Data(Elements.size());
    std::transform(
        Elements.begin(), Elements.end(), Data.begin(),
        [&C](const uint64_t Element) { return C.getInt64(Element).unwrap(); });
    return LLVMConstVector(Data.data(), static_cast<unsigned int>(Data.size()));
  }

#define DECLARE_VALUE_CHECK(name)                                              \
  bool isA##name() const noexcept { return LLVMIsA##name(Ref) != nullptr; }
  LLVM_FOR_EACH_VALUE_SUBCLASS(DECLARE_VALUE_CHECK)
#undef DECLARE_VALUE_CHECK

  inline void addFnAttr(const Attribute &A) noexcept;
  inline void addParamAttr(unsigned Index, const Attribute &A) noexcept;
  inline void addCallSiteAttribute(const Attribute &A) noexcept;
  inline void setMetadata(Context &C, unsigned int KindID,
                          Metadata Node) noexcept;

  Value getFirstParam() noexcept { return LLVMGetFirstParam(Ref); }
  Value getNextParam() noexcept { return LLVMGetNextParam(Ref); }
  Value getNextGlobal() noexcept { return LLVMGetNextGlobal(Ref); }
  Value getNextFunction() noexcept { return LLVMGetNextFunction(Ref); }

  Type getType() const noexcept { return LLVMTypeOf(Ref); }
  void setInitializer(Value ConstantVal) noexcept {
    LLVMSetInitializer(Ref, ConstantVal.unwrap());
  }
  void setGlobalConstant(bool IsConstant) noexcept {
    LLVMSetGlobalConstant(Ref, IsConstant);
  }
  LLVMLinkage getLinkage() noexcept { return LLVMGetLinkage(Ref); }
  void setLinkage(LLVMLinkage Linkage) noexcept {
    LLVMSetLinkage(Ref, Linkage);
  }
  void setVisibility(LLVMVisibility Viz) noexcept {
    LLVMSetVisibility(Ref, Viz);
  }
  inline void setDSOLocal(bool Local) noexcept;
  void setDLLStorageClass(LLVMDLLStorageClass Class) noexcept {
    LLVMSetDLLStorageClass(Ref, Class);
  }
  bool getVolatile() noexcept { return LLVMGetVolatile(Ref); }
  void setVolatile(bool IsVolatile) noexcept {
    LLVMSetVolatile(Ref, IsVolatile);
  }
  bool getWeak() noexcept { return LLVMGetWeak(Ref); }
  void setWeak(bool IsWeak) noexcept { LLVMSetWeak(Ref, IsWeak); }
  unsigned int getAlignment() noexcept { return LLVMGetAlignment(Ref); }
  void setAlignment(unsigned int Bytes) noexcept {
    LLVMSetAlignment(Ref, Bytes);
  }
  LLVMAtomicOrdering getOrdering() noexcept { return LLVMGetOrdering(Ref); }
  void setOrdering(LLVMAtomicOrdering Ordering) noexcept {
    LLVMSetOrdering(Ref, Ordering);
  }

  inline void addCase(Value OnVal, BasicBlock Dest) noexcept;
  inline void addDestination(BasicBlock Dest) noexcept;
  inline void addIncoming(Value IncomingValue,
                          BasicBlock IncomingBlocks) noexcept;
  inline void addIncoming(Span<const Value> IncomingValue,
                          Span<const BasicBlock> IncomingBlocks) noexcept;
  inline void eliminateUnreachableBlocks() noexcept;

private:
  LLVMValueRef Ref = nullptr;
};

struct FunctionCallee {
  FunctionCallee() = default;
  FunctionCallee(Type T, Value F) : Ty(T), Fn(F) {}
  Type Ty;
  Value Fn;
};

class Metadata {
public:
  constexpr Metadata(LLVMMetadataRef R) noexcept : Ref(R) {}
  Metadata(const Metadata &) = delete;
  Metadata &operator=(const Metadata &) = delete;
  Metadata(Metadata &&M) noexcept : Metadata() { swap(*this, M); }
  Metadata &operator=(Metadata &&M) noexcept {
    swap(*this, M);
    return *this;
  }
  Metadata(Context &C, Span<Metadata> M) noexcept {
    const auto Data = const_cast<LLVMMetadataRef *>(
        reinterpret_cast<const LLVMMetadataRef *>(M.data()));
    const auto Size = static_cast<unsigned int>(M.size());
    Ref = LLVMMDNodeInContext2(C.unwrap(), Data, Size);
  }
  Metadata(Value V) noexcept : Ref(LLVMValueAsMetadata(V.unwrap())) {}

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto unwrap() noexcept { return Ref; }
  friend void swap(Metadata &LHS, Metadata &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

private:
  constexpr Metadata() noexcept = default;
  LLVMMetadataRef Ref = nullptr;
};

class Attribute {
public:
  constexpr Attribute(LLVMAttributeRef R) noexcept : Ref(R) {}
  Attribute(const Attribute &) = delete;
  Attribute &operator=(const Attribute &) = delete;
  Attribute(Attribute &&M) noexcept : Attribute() { swap(*this, M); }
  Attribute &operator=(Attribute &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto unwrap() noexcept { return Ref; }
  friend void swap(Attribute &LHS, Attribute &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static Attribute createEnum(Context &C, unsigned int KindID,
                              uint64_t Val) noexcept {
    return LLVMCreateEnumAttribute(C.unwrap(), KindID, Val);
  }
  static Attribute createString(Context &C, std::string_view KindID,
                                std::string_view Val) noexcept {
    return LLVMCreateStringAttribute(
        C.unwrap(), KindID.data(), static_cast<unsigned int>(KindID.size()),
        Val.data(), static_cast<unsigned int>(Val.size()));
  }

private:
  constexpr Attribute() noexcept = default;
  LLVMAttributeRef Ref = nullptr;
};

Value Module::addFunction(Type Ty, LLVMLinkage Linkage,
                          const char *Name) noexcept {
  Value Fn = LLVMAddFunction(Ref, Name, Ty.unwrap());
  Fn.setLinkage(Linkage);
  return Fn;
}

Value Module::addGlobal(Type Ty, bool IsConstant, LLVMLinkage Linkage,
                        Value Initializer, const char *Name) noexcept {
  Value G = LLVMAddGlobal(Ref, Ty.unwrap(), Name);
  G.setLinkage(Linkage);
  G.setGlobalConstant(IsConstant);
  G.setInitializer(Initializer);
  return G;
}

Value Module::getNamedGlobal(const char *Name) noexcept {
  return LLVMGetNamedGlobal(Ref, Name);
}

Value Module::addAlias(Type Ty, Value V, const char *Name,
                       unsigned int AddrSpace [[maybe_unused]]) noexcept {
#if LLVM_VERSION_MAJOR >= 14
  return LLVMAddAlias2(Ref, Ty.unwrap(), AddrSpace, V.unwrap(), Name);
#else
  return LLVMAddAlias(Ref, Ty.unwrap(), V.unwrap(), Name);
#endif
}

void Module::addFlag(LLVMModuleFlagBehavior Behavior, std::string_view Key,
                     const Metadata &Val) noexcept {
  LLVMAddModuleFlag(Ref, Behavior, Key.data(), Key.size(), Val.unwrap());
}

void Module::addFlag(LLVMModuleFlagBehavior Behavior, std::string_view Key,
                     const Value &Val) noexcept {
  LLVMAddModuleFlag(Ref, Behavior, Key.data(), Key.size(),
                    Metadata(Val).unwrap());
}

void Module::addFlag(LLVMModuleFlagBehavior Behavior, std::string_view Key,
                     uint32_t Val) noexcept {
  Type Int32Ty = LLVMInt32TypeInContext(LLVMGetModuleContext(Ref));
  LLVMAddModuleFlag(Ref, Behavior, Key.data(), Key.size(),
                    Metadata(Value::getConstInt(Int32Ty, Val)).unwrap());
}

Value Module::getFirstGlobal() noexcept { return LLVMGetFirstGlobal(Ref); }
Value Module::getFirstFunction() noexcept { return LLVMGetFirstFunction(Ref); }

Message Module::printModuleToFile(const char *Filename) noexcept {
  Message M;
  LLVMPrintModuleToFile(Ref, Filename, &M.unwrap());
  return M;
}

Message Module::verify(LLVMVerifierFailureAction Action) noexcept {
  Message M;
  LLVMVerifyModule(Ref, Action, &M.unwrap());
  return M;
}

Type Context::getVoidTy() noexcept { return LLVMVoidTypeInContext(Ref); }
Type Context::getInt1Ty() noexcept { return LLVMInt1TypeInContext(Ref); }
Type Context::getInt8Ty() noexcept { return LLVMInt8TypeInContext(Ref); }
Type Context::getInt16Ty() noexcept { return LLVMInt16TypeInContext(Ref); }
Type Context::getInt32Ty() noexcept { return LLVMInt32TypeInContext(Ref); }
Type Context::getInt64Ty() noexcept { return LLVMInt64TypeInContext(Ref); }
Type Context::getInt128Ty() noexcept { return LLVMInt128TypeInContext(Ref); }
Type Context::getIntNTy(unsigned int NumBits) noexcept {
  return LLVMIntTypeInContext(Ref, NumBits);
}
Type Context::getFloatTy() noexcept { return LLVMFloatTypeInContext(Ref); }
Type Context::getDoubleTy() noexcept { return LLVMDoubleTypeInContext(Ref); }

Value Context::getFalse() noexcept {
  return Value::getConstInt(getInt1Ty(), 0);
}
Value Context::getTrue() noexcept { return Value::getConstInt(getInt1Ty(), 1); }
Value Context::getInt8(uint8_t C) noexcept {
  return Value::getConstInt(getInt8Ty(), C);
}
Value Context::getInt16(uint16_t C) noexcept {
  return Value::getConstInt(getInt16Ty(), C);
}
Value Context::getInt32(uint32_t C) noexcept {
  return Value::getConstInt(getInt32Ty(), C);
}
Value Context::getInt64(uint64_t C) noexcept {
  return Value::getConstInt(getInt64Ty(), C);
}
Value Context::getFloat(float C) noexcept {
  return Value::getConstReal(getFloatTy(), C);
}
Value Context::getDouble(double C) noexcept {
  return Value::getConstReal(getDoubleTy(), C);
}

void Value::addFnAttr(const Attribute &A) noexcept {
  LLVMAddAttributeAtIndex(
      Ref, static_cast<unsigned int>(LLVMAttributeFunctionIndex), A.unwrap());
}
void Value::addParamAttr(unsigned Index, const Attribute &A) noexcept {
  LLVMAddAttributeAtIndex(Ref, 1 + Index, A.unwrap());
}
void Value::addCallSiteAttribute(const Attribute &A) noexcept {
  LLVMAddCallSiteAttribute(
      Ref, static_cast<unsigned int>(LLVMAttributeFunctionIndex), A.unwrap());
}
void Value::setMetadata(Context &C, unsigned int KindID,
                        Metadata Node) noexcept {
  LLVMSetMetadata(Ref, KindID, LLVMMetadataAsValue(C.unwrap(), Node.unwrap()));
}

static inline Message getDefaultTargetTriple() noexcept {
  return LLVMGetDefaultTargetTriple();
}
static inline Message getHostCPUName() noexcept { return LLVMGetHostCPUName(); }
static inline Message getHostCPUFeatures() noexcept {
  return LLVMGetHostCPUFeatures();
}

class BasicBlock {
public:
  constexpr BasicBlock() noexcept = default;
  constexpr BasicBlock(LLVMBasicBlockRef R) noexcept : Ref(R) {}
  BasicBlock(const BasicBlock &) = default;
  BasicBlock &operator=(const BasicBlock &) = default;
  BasicBlock(BasicBlock &&B) noexcept : BasicBlock() { swap(*this, B); }
  BasicBlock &operator=(BasicBlock &&B) noexcept {
    swap(*this, B);
    return *this;
  }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(BasicBlock &LHS, BasicBlock &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static BasicBlock create(Context &C, Value F, const char *Name) noexcept {
    return LLVMAppendBasicBlockInContext(C.unwrap(), F.unwrap(), Name);
  }
  void erase() noexcept { LLVMDeleteBasicBlock(Ref); }

private:
  LLVMBasicBlockRef Ref = nullptr;
};

void Value::addCase(Value OnVal, BasicBlock Dest) noexcept {
  LLVMAddCase(Ref, OnVal.unwrap(), Dest.unwrap());
}

void Value::addDestination(BasicBlock Dest) noexcept {
  LLVMAddDestination(Ref, Dest.unwrap());
}

void Value::addIncoming(Value IncomingValue,
                        BasicBlock IncomingBlocks) noexcept {
  LLVMAddIncoming(Ref, &IncomingValue.unwrap(), &IncomingBlocks.unwrap(), 1);
}

void Value::addIncoming(Span<const Value> IncomingValue,
                        Span<const BasicBlock> IncomingBlocks) noexcept {
  assuming(IncomingBlocks.size() == IncomingValue.size());
  const auto ValueData = const_cast<LLVMValueRef *>(
      reinterpret_cast<const LLVMValueRef *>(IncomingValue.data()));
  const auto BlockData = const_cast<LLVMBasicBlockRef *>(
      reinterpret_cast<const LLVMBasicBlockRef *>(IncomingBlocks.data()));
  const auto Size = static_cast<unsigned int>(IncomingValue.size());
  LLVMAddIncoming(Ref, ValueData, BlockData, Size);
}

class Builder {
private:
  LLVMValueRef getFn() noexcept {
    return LLVMGetBasicBlockParent(LLVMGetInsertBlock(Ref));
  }
  LLVMModuleRef getMod() noexcept { return LLVMGetGlobalParent(getFn()); }
  LLVMContextRef getCtx() noexcept { return LLVMGetModuleContext(getMod()); }

public:
  constexpr Builder() noexcept = default;
  constexpr Builder(LLVMBuilderRef R) noexcept : Ref(R) {}
  Builder(const Builder &) = delete;
  Builder &operator=(const Builder &) = delete;
  Builder(Builder &&B) noexcept : Builder() { swap(*this, B); }
  Builder &operator=(Builder &&B) noexcept {
    swap(*this, B);
    return *this;
  }
  ~Builder() noexcept { LLVMDisposeBuilder(Ref); }

  Builder(Context &C) noexcept : Ref(LLVMCreateBuilderInContext(C.unwrap())) {}

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(Builder &LHS, Builder &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  void positionAtEnd(BasicBlock B) noexcept {
    LLVMPositionBuilderAtEnd(Ref, B.unwrap());
  }
  BasicBlock getInsertBlock() noexcept { return LLVMGetInsertBlock(Ref); }

  Value createRetVoid() noexcept { return LLVMBuildRetVoid(Ref); }
  Value createRet(Value V) noexcept { return LLVMBuildRet(Ref, V.unwrap()); }
  Value createAggregateRet(Span<const Value> RetVals) noexcept {
    const auto Data = const_cast<LLVMValueRef *>(
        reinterpret_cast<const LLVMValueRef *>(RetVals.data()));
    const auto Size = static_cast<unsigned int>(RetVals.size());
    return LLVMBuildAggregateRet(Ref, Data, Size);
  }
  Value createBr(BasicBlock Dest) noexcept {
    return LLVMBuildBr(Ref, Dest.unwrap());
  }
  Value createCondBr(Value If, BasicBlock Then, BasicBlock Else) noexcept {
    return LLVMBuildCondBr(Ref, If.unwrap(), Then.unwrap(), Else.unwrap());
  }
  Value createSwitch(Value V, BasicBlock Else, unsigned int NumCases) noexcept {
    return LLVMBuildSwitch(Ref, V.unwrap(), Else.unwrap(), NumCases);
  }
  Value createIndirectBr(Value Addr, unsigned int NumDests) noexcept {
    return LLVMBuildIndirectBr(Ref, Addr.unwrap(), NumDests);
  }
  Value createUnreachable() noexcept { return LLVMBuildUnreachable(Ref); }

  Value createAdd(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildAdd(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNSWAdd(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildNSWAdd(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNUWAdd(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildNUWAdd(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFAdd(Value LHS, Value RHS, const char *Name = "") noexcept {
    Value Ret = createIntrinsic(
        Core::ExperimentalConstrainedFAdd, {LHS.getType()},
        {LHS, RHS, getConstrainedFPRounding(), getConstrainedFPExcept()}, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createSub(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildSub(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNSWSub(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildNSWSub(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNUWSub(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildNUWSub(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFSub(Value LHS, Value RHS, const char *Name = "") noexcept {
    Value Ret = createIntrinsic(
        Core::ExperimentalConstrainedFSub, {LHS.getType()},
        {LHS, RHS, getConstrainedFPRounding(), getConstrainedFPExcept()}, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createMul(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildMul(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNSWMul(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildNSWMul(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNUWMul(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildNUWMul(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFMul(Value LHS, Value RHS, const char *Name = "") noexcept {
    Value Ret = createIntrinsic(
        Core::ExperimentalConstrainedFMul, {LHS.getType()},
        {LHS, RHS, getConstrainedFPRounding(), getConstrainedFPExcept()}, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createUDiv(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildUDiv(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createExactUDiv(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildExactUDiv(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createSDiv(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildSDiv(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createExactSDiv(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildExactSDiv(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFDiv(Value LHS, Value RHS, const char *Name = "") noexcept {
    Value Ret = createIntrinsic(
        Core::ExperimentalConstrainedFDiv, {LHS.getType()},
        {LHS, RHS, getConstrainedFPRounding(), getConstrainedFPExcept()}, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createURem(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildURem(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createSRem(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildSRem(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createShl(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildShl(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createLShr(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildLShr(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createAShr(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildAShr(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createAnd(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildAnd(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createOr(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildOr(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createXor(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildXor(Ref, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createNeg(Value V, const char *Name = "") noexcept {
    return LLVMBuildNeg(Ref, V.unwrap(), Name);
  }
  Value createNSWNeg(Value V, const char *Name = "") noexcept {
    return LLVMBuildNSWNeg(Ref, V.unwrap(), Name);
  }
  Value createNUWNeg(Value V, const char *Name = "") noexcept {
    return LLVMBuildNUWNeg(Ref, V.unwrap(), Name);
  }
  Value createFNeg(Value V, const char *Name = "") noexcept {
    return LLVMBuildFNeg(Ref, V.unwrap(), Name);
  }
  Value createNot(Value V, const char *Name = "") noexcept {
    return LLVMBuildNot(Ref, V.unwrap(), Name);
  }
  Value createAlloca(Type Ty, const char *Name = "") noexcept {
    return LLVMBuildAlloca(Ref, Ty.unwrap(), Name);
  }
  Value createArrayAlloca(Type Ty, Value Val, const char *Name = "") noexcept {
    return LLVMBuildArrayAlloca(Ref, Ty.unwrap(), Val.unwrap(), Name);
  }
  Value createLoad(Type Ty, Value PointerVal, bool Volatile = false,
                   const char *Name = "") noexcept {
    auto Ret = LLVMBuildLoad2(Ref, Ty.unwrap(), PointerVal.unwrap(), Name);
    if (Volatile) {
      LLVMSetVolatile(Ret, true);
    }
    return Ret;
  }
  Value createStore(Value Val, Value Ptr, bool Volatile = false) noexcept {
    auto Ret = LLVMBuildStore(Ref, Val.unwrap(), Ptr.unwrap());
    if (Volatile) {
      LLVMSetVolatile(Ret, true);
    }
    return Ret;
  }

  Value createInBoundsGEP1(Type Ty, Value Pointer, Value Idx0,
                           const char *Name = "") noexcept {
    LLVMValueRef Data[1] = {Idx0.unwrap()};
    return LLVMBuildInBoundsGEP2(Ref, Ty.unwrap(), Pointer.unwrap(), Data,
                                 std::size(Data), Name);
  }
  Value createInBoundsGEP2(Type Ty, Value Pointer, Value Idx0, Value Idx1,
                           const char *Name = "") noexcept {
    LLVMValueRef Data[2] = {Idx0.unwrap(), Idx1.unwrap()};
    return LLVMBuildInBoundsGEP2(Ref, Ty.unwrap(), Pointer.unwrap(), Data,
                                 std::size(Data), Name);
  }
  Value createConstInBoundsGEP1_64(Type Ty, Value Pointer, uint64_t Idx0,
                                   const char *Name = "") noexcept {
    Type Int64Ty = LLVMInt64TypeInContext(LLVMGetTypeContext(Ty.unwrap()));
    LLVMValueRef Data[1] = {Value::getConstInt(Int64Ty, Idx0).unwrap()};
    return LLVMBuildInBoundsGEP2(Ref, Ty.unwrap(), Pointer.unwrap(), Data,
                                 std::size(Data), Name);
  }

  Value createConstInBoundsGEP2_64(Type Ty, Value Pointer, uint64_t Idx0,
                                   uint64_t Idx1,
                                   const char *Name = "") noexcept {
    Type Int64Ty = LLVMInt64TypeInContext(LLVMGetTypeContext(Ty.unwrap()));
    LLVMValueRef Data[2] = {Value::getConstInt(Int64Ty, Idx0).unwrap(),
                            Value::getConstInt(Int64Ty, Idx1).unwrap()};
    return LLVMBuildInBoundsGEP2(Ref, Ty.unwrap(), Pointer.unwrap(), Data,
                                 std::size(Data), Name);
  }

  Value createTrunc(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildTrunc(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createZExt(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildZExt(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createSExt(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildSExt(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createFPToUI(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildFPToUI(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createFPToSI(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildFPToSI(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createUIToFP(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildUIToFP(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createSIToFP(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildSIToFP(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createFPTrunc(Value Val, Type DestTy, const char *Name = "") noexcept {
    Value Ret = createIntrinsic(
        Core::ExperimentalConstrainedFPTrunc, {DestTy, Val.getType()},
        {Val, getConstrainedFPRounding(), getConstrainedFPExcept()}, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createFPExt(Value Val, Type DestTy, const char *Name = "") noexcept {
    Value Ret = createIntrinsic(Core::ExperimentalConstrainedFPExt,
                                {DestTy, Val.getType()},
                                {Val, getConstrainedFPExcept()}, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createPtrToInt(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildPtrToInt(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createIntToPtr(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildIntToPtr(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createBitCast(Value Val, Type DestTy, const char *Name = "") noexcept {
    return LLVMBuildBitCast(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createZExtOrBitCast(Value Val, Type DestTy,
                            const char *Name = "") noexcept {
    return LLVMBuildZExtOrBitCast(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createSExtOrBitCast(Value Val, Type DestTy,
                            const char *Name = "") noexcept {
    return LLVMBuildSExtOrBitCast(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createTruncOrBitCast(Value Val, Type DestTy,
                             const char *Name = "") noexcept {
    return LLVMBuildTruncOrBitCast(Ref, Val.unwrap(), DestTy.unwrap(), Name);
  }
  Value createZExtOrTrunc(Value Val, Type DestTy,
                          const char *Name = "") noexcept {
    const auto VTy = Val.getType();
    assuming(DestTy.isIntegerTy() || DestTy.isVectorTy());
    assuming(VTy.isIntegerTy() || VTy.isVectorTy());
    const auto VTySize = VTy.getPrimitiveSizeInBits();
    const auto DestTySize = DestTy.getPrimitiveSizeInBits();
    if (VTySize < DestTySize) {
      return createZExt(Val, DestTy, Name);
    }
    if (VTySize > DestTySize) {
      return createTrunc(Val, DestTy, Name);
    }
    return Val;
  }
  Value createSExtOrTrunc(Value Val, Type DestTy,
                          const char *Name = "") noexcept {
    const auto VTy = Val.getType();
    assuming(DestTy.isIntegerTy() || DestTy.isVectorTy());
    assuming(VTy.isIntegerTy() || VTy.isVectorTy());
    const auto VTySize = VTy.getPrimitiveSizeInBits();
    const auto DestTySize = DestTy.getPrimitiveSizeInBits();
    if (VTySize < DestTySize) {
      return createSExt(Val, DestTy, Name);
    }
    if (VTySize > DestTySize) {
      return createTrunc(Val, DestTy, Name);
    }
    return Val;
  }

  Value createICmp(LLVMIntPredicate Op, Value LHS, Value RHS,
                   const char *Name = "") noexcept {
    return LLVMBuildICmp(Ref, Op, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createICmpEQ(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntEQ, LHS, RHS, Name);
  }
  Value createICmpNE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntNE, LHS, RHS, Name);
  }
  Value createICmpUGT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntUGT, LHS, RHS, Name);
  }
  Value createICmpUGE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntUGE, LHS, RHS, Name);
  }
  Value createICmpULT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntULT, LHS, RHS, Name);
  }
  Value createICmpULE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntULE, LHS, RHS, Name);
  }
  Value createICmpSGT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntSGT, LHS, RHS, Name);
  }
  Value createICmpSGE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntSGE, LHS, RHS, Name);
  }
  Value createICmpSLT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntSLT, LHS, RHS, Name);
  }
  Value createICmpSLE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return createICmp(LLVMIntSLE, LHS, RHS, Name);
  }
  Value createFCmp(LLVMRealPredicate Op, Value LHS, Value RHS,
                   const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, Op, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpOEQ(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealOEQ, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpOGT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealOGT, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpOGE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealOGE, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpOLT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealOLT, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpOLE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealOLE, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpONE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealONE, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpORD(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealORD, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpUNO(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealUNO, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpUEQ(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealUEQ, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpUGT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealUGT, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpUGE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealUGE, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpULT(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealULT, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpULE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealULE, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createFCmpUNE(Value LHS, Value RHS, const char *Name = "") noexcept {
    return LLVMBuildFCmp(Ref, LLVMRealUNE, LHS.unwrap(), RHS.unwrap(), Name);
  }
  Value createPHI(Type Ty, const char *Name = "") noexcept {
    return LLVMBuildPhi(Ref, Ty.unwrap(), Name);
  }
  Value createCall(FunctionCallee Callee, Span<const Value> Args,
                   const char *Name = "") noexcept {
    const auto Data = const_cast<LLVMValueRef *>(
        reinterpret_cast<const LLVMValueRef *>(Args.data()));
    const auto Size = static_cast<unsigned int>(Args.size());
    Value Ret = LLVMBuildCall2(Ref, Callee.Ty.unwrap(), Callee.Fn.unwrap(),
                               Data, Size, Name);
    Ret.addCallSiteAttribute(
        LLVMCreateEnumAttribute(getCtx(), LLVM::Core::StrictFP, 0));
    return Ret;
  }
  Value createSelect(Value If, Value Then, Value Else,
                     const char *Name = "") noexcept {
    return LLVMBuildSelect(Ref, If.unwrap(), Then.unwrap(), Else.unwrap(),
                           Name);
  }
  Value createExtractElement(Value VecVal, Value Index,
                             const char *Name = "") noexcept {
    return LLVMBuildExtractElement(Ref, VecVal.unwrap(), Index.unwrap(), Name);
  }
  Value createInsertElement(Value VecVal, Value EltVal, Value Index,
                            const char *Name = "") noexcept {
    return LLVMBuildInsertElement(Ref, VecVal.unwrap(), EltVal.unwrap(),
                                  Index.unwrap(), Name);
  }
  Value createShuffleVector(Value V1, Value V2, Value Mask,
                            const char *Name = "") noexcept {
    return LLVMBuildShuffleVector(Ref, V1.unwrap(), V2.unwrap(), Mask.unwrap(),
                                  Name);
  }
  Value createExtractValue(Value AggVal, unsigned int Index,
                           const char *Name = "") noexcept {
    return LLVMBuildExtractValue(Ref, AggVal.unwrap(), Index, Name);
  }
  Value createInsertValue(Value AggVal, Value EltVal, unsigned int Index,
                          const char *Name = "") noexcept {
    return LLVMBuildInsertValue(Ref, AggVal.unwrap(), EltVal.unwrap(), Index,
                                Name);
  }

  Value createIsNull(Value Val, const char *Name = "") noexcept {
    return LLVMBuildIsNull(Ref, Val.unwrap(), Name);
  }
  Value createIsNotNull(Value Val, const char *Name = "") noexcept {
    return LLVMBuildIsNotNull(Ref, Val.unwrap(), Name);
  }
  Value createFence(LLVMAtomicOrdering Ordering, bool SingleThread = false,
                    const char *Name = "") noexcept {
    return LLVMBuildFence(Ref, Ordering, SingleThread, Name);
  }
  Value createAtomicRMW(LLVMAtomicRMWBinOp Op, Value Ptr, Value Val,
                        LLVMAtomicOrdering Ordering,
                        bool SingleThread = false) noexcept {
    return LLVMBuildAtomicRMW(Ref, Op, Ptr.unwrap(), Val.unwrap(), Ordering,
                              SingleThread);
  }
  Value createAtomicCmpXchg(Value Ptr, Value Cmp, Value New,
                            LLVMAtomicOrdering SuccessOrdering,
                            LLVMAtomicOrdering FailureOrdering,
                            bool SingleThread = false) noexcept {
    return LLVMBuildAtomicCmpXchg(Ref, Ptr.unwrap(), Cmp.unwrap(), New.unwrap(),
                                  SuccessOrdering, FailureOrdering,
                                  SingleThread);
  }

  Value createIntrinsic(unsigned int ID, Span<const Type> Types,
                        Span<const Value> Args,
                        const char *Name = "") noexcept {
    FunctionCallee C;
    {
      const auto Data = const_cast<LLVMTypeRef *>(
          reinterpret_cast<const LLVMTypeRef *>(Types.data()));
      const auto Size = static_cast<unsigned int>(Types.size());
      C.Fn = LLVMGetIntrinsicDeclaration(getMod(), ID, Data, Size);
      C.Ty = LLVMIntrinsicGetType(getCtx(), ID, Data, Size);
    }
    return createCall(C, Args, Name);
  }
  Value createUnaryIntrinsic(unsigned int ID, Value V,
                             const char *Name = "") noexcept {
    FunctionCallee C;
    {
      LLVMTypeRef ParamTypes[1] = {V.getType().unwrap()};
      C.Fn = LLVMGetIntrinsicDeclaration(getMod(), ID, ParamTypes, 1);
      C.Ty = LLVMIntrinsicGetType(getCtx(), ID, ParamTypes, 1);
    }
    return createCall(C, {V}, Name);
  }
  Value createBinaryIntrinsic(unsigned int ID, Value LHS, Value RHS,
                              const char *Name = "") noexcept {
    FunctionCallee C;
    {
      LLVMTypeRef ParamTypes[2] = {LHS.getType().unwrap(),
                                   RHS.getType().unwrap()};
      C.Fn = LLVMGetIntrinsicDeclaration(getMod(), ID, ParamTypes, 2);
      C.Ty = LLVMIntrinsicGetType(getCtx(), ID, ParamTypes, 2);
    }
    return createCall(C, {LHS, RHS}, Name);
  }

  Value createVectorSplat(unsigned int ElementCount, Value V,
                          const char *Name = "") noexcept {
    Value Zero = Value::getConstInt(LLVMInt32TypeInContext(getCtx()), 0);
    auto Empty =
        Value::getUndef(Type::getVectorType(V.getType(), ElementCount));
    auto One = createInsertElement(Empty, V, Zero);
    std::vector<Value> Mask(ElementCount, Zero);
    return createShuffleVector(One, Empty, Value::getConstVector(Mask), Name);
  }

  Value createLikely(Value V) noexcept {
    Type Int1Ty = LLVMInt1TypeInContext(getCtx());
    return createIntrinsic(LLVM::Core::Expect, {Int1Ty},
                           {V, Value::getConstInt(Int1Ty, 1)});
  }

  Value getConstrainedFPRounding() noexcept {
    using namespace std::literals;
    auto Ctx = getCtx();
    auto RoundingStr = "round.tonearest"sv;
    auto RoundingMDS =
        LLVMMDStringInContext2(Ctx, RoundingStr.data(), RoundingStr.size());
    return LLVMMetadataAsValue(Ctx, RoundingMDS);
  }
  Value getConstrainedFPExcept() noexcept {
    using namespace std::literals;
    auto Ctx = getCtx();
    auto ExceptStr = "fpexcept.ignore"sv;
    auto ExceptMDS =
        LLVMMDStringInContext2(Ctx, ExceptStr.data(), ExceptStr.size());
    return LLVMMetadataAsValue(Ctx, ExceptMDS);
  }

private:
  LLVMBuilderRef Ref = nullptr;
};

class Target {
public:
  constexpr Target() noexcept = default;
  constexpr Target(LLVMTargetRef R) noexcept : Ref(R) {}
  Target(const Target &) = delete;
  Target &operator=(const Target &) = delete;
  Target(Target &&M) noexcept : Target() { swap(*this, M); }
  Target &operator=(Target &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(Target &LHS, Target &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static std::pair<Target, Message> getFromTriple(const char *Triple) noexcept {
    std::pair<Target, Message> Result;
    LLVMGetTargetFromTriple(Triple, &Result.first.unwrap(),
                            &Result.second.unwrap());
    return Result;
  }

private:
  LLVMTargetRef Ref = nullptr;
};

class PassManager {
public:
  constexpr PassManager() noexcept = default;
  constexpr PassManager(LLVMPassManagerRef R) noexcept : Ref(R) {}
  PassManager(const PassManager &) = delete;
  PassManager &operator=(const PassManager &) = delete;
  PassManager(PassManager &&M) noexcept : PassManager() { swap(*this, M); }
  PassManager &operator=(PassManager &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  ~PassManager() noexcept { LLVMDisposePassManager(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(PassManager &LHS, PassManager &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static PassManager create() noexcept { return LLVMCreatePassManager(); }

  static PassManager createForModule(Module &M) noexcept {
    return LLVMCreateFunctionPassManagerForModule(M.unwrap());
  }

  void addTailCallEliminationPass() noexcept {
    LLVMAddTailCallEliminationPass(Ref);
  }
  void addAlwaysInlinerPass() noexcept { LLVMAddAlwaysInlinerPass(Ref); }

  void initializeFunctionPassManager() noexcept {
    LLVMInitializeFunctionPassManager(Ref);
  }
  void finalizeFunctionPassManager() noexcept {
    LLVMFinalizeFunctionPassManager(Ref);
  }
  void runFunctionPassManager(Value F) noexcept {
    LLVMRunFunctionPassManager(Ref, F.unwrap());
  }
  void runPassManager(Module &M) noexcept {
    LLVMRunPassManager(Ref, M.unwrap());
  }

private:
  LLVMPassManagerRef Ref = nullptr;
};

class PassManagerBuilder {
public:
  constexpr PassManagerBuilder() noexcept = default;
  constexpr PassManagerBuilder(LLVMPassManagerBuilderRef R) noexcept : Ref(R) {}
  PassManagerBuilder(const PassManagerBuilder &) = delete;
  PassManagerBuilder &operator=(const PassManagerBuilder &) = delete;
  PassManagerBuilder(PassManagerBuilder &&M) noexcept : PassManagerBuilder() {
    swap(*this, M);
  }
  PassManagerBuilder &operator=(PassManagerBuilder &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  ~PassManagerBuilder() noexcept { LLVMPassManagerBuilderDispose(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(PassManagerBuilder &LHS, PassManagerBuilder &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static PassManagerBuilder create() noexcept {
    return LLVMPassManagerBuilderCreate();
  }
  void setOptLevel(unsigned int OptLevel) noexcept {
    LLVMPassManagerBuilderSetOptLevel(Ref, OptLevel);
  }
  void setSizeLevel(unsigned int SizeLevel) noexcept {
    LLVMPassManagerBuilderSetSizeLevel(Ref, SizeLevel);
  }
  void populateFunctionPassManager(PassManager &P) noexcept {
    LLVMPassManagerBuilderPopulateFunctionPassManager(Ref, P.unwrap());
  }
  void populateModulePassManager(PassManager &P) noexcept {
    LLVMPassManagerBuilderPopulateModulePassManager(Ref, P.unwrap());
  }

private:
  LLVMPassManagerBuilderRef Ref = nullptr;
};

class TargetMachine {
public:
  constexpr TargetMachine() noexcept = default;
  constexpr TargetMachine(LLVMTargetMachineRef R) noexcept : Ref(R) {}
  TargetMachine(const TargetMachine &) = delete;
  TargetMachine &operator=(const TargetMachine &) = delete;
  TargetMachine(TargetMachine &&M) noexcept : TargetMachine() {
    swap(*this, M);
  }
  TargetMachine &operator=(TargetMachine &&M) noexcept {
    swap(*this, M);
    return *this;
  }

  ~TargetMachine() noexcept { LLVMDisposeTargetMachine(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(TargetMachine &LHS, TargetMachine &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static TargetMachine create(Target &T, const char *Triple, const char *CPU,
                              const char *Features, LLVMCodeGenOptLevel Level,
                              LLVMRelocMode Reloc,
                              LLVMCodeModel CodeModel) noexcept {
    return LLVMCreateTargetMachine(T.unwrap(), Triple, CPU, Features, Level,
                                   Reloc, CodeModel);
  }

  void addAnalysisPasses(PassManager &P) noexcept {
    LLVMAddAnalysisPasses(Ref, P.unwrap());
  }

  std::pair<MemoryBuffer, Message>
  emitToMemoryBuffer(Module &M, LLVMCodeGenFileType CodeGen) noexcept {
    std::pair<MemoryBuffer, Message> Result;
    LLVMTargetMachineEmitToMemoryBuffer(Ref, M.unwrap(), CodeGen,
                                        &Result.second.unwrap(),
                                        &Result.first.unwrap());
    return Result;
  }

private:
  LLVMTargetMachineRef Ref = nullptr;
};

class SectionIterator {
public:
  constexpr SectionIterator() noexcept = default;
  constexpr SectionIterator(LLVMSectionIteratorRef R) noexcept : Ref(R) {}
  SectionIterator(const SectionIterator &) = delete;
  SectionIterator &operator=(const SectionIterator &) = delete;
  SectionIterator(SectionIterator &&B) noexcept : SectionIterator() {
    swap(*this, B);
  }
  SectionIterator &operator=(SectionIterator &&B) noexcept {
    swap(*this, B);
    return *this;
  }

  ~SectionIterator() noexcept { LLVMDisposeSectionIterator(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(SectionIterator &LHS, SectionIterator &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  void next() const noexcept { LLVMMoveToNextSection(Ref); }

  const char *getName() const noexcept { return LLVMGetSectionName(Ref); }
  uint64_t getAddress() const noexcept { return LLVMGetSectionAddress(Ref); }
  uint64_t getSize() const noexcept { return LLVMGetSectionSize(Ref); }
  Span<const uint8_t> getContents() const noexcept {
    return {reinterpret_cast<const uint8_t *>(LLVMGetSectionContents(Ref)),
            LLVMGetSectionSize(Ref)};
  }

  inline bool isText() const noexcept;
  inline bool isData() const noexcept;
  inline bool isBSS() const noexcept;
  inline bool isPData() const noexcept;

private:
  LLVMSectionIteratorRef Ref = nullptr;
};

class SymbolIterator {
public:
  constexpr SymbolIterator() noexcept = default;
  constexpr SymbolIterator(LLVMSymbolIteratorRef R) noexcept : Ref(R) {}
  SymbolIterator(const SymbolIterator &) = delete;
  SymbolIterator &operator=(const SymbolIterator &) = delete;
  SymbolIterator(SymbolIterator &&B) noexcept : SymbolIterator() {
    swap(*this, B);
  }
  SymbolIterator &operator=(SymbolIterator &&B) noexcept {
    swap(*this, B);
    return *this;
  }

  ~SymbolIterator() noexcept { LLVMDisposeSymbolIterator(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(SymbolIterator &LHS, SymbolIterator &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  void next() const noexcept { LLVMMoveToNextSymbol(Ref); }

  const char *getName() const noexcept { return LLVMGetSymbolName(Ref); }
  uint64_t getAddress() const noexcept { return LLVMGetSymbolAddress(Ref); }
  uint64_t getSize() const noexcept { return LLVMGetSymbolSize(Ref); }

private:
  LLVMSymbolIteratorRef Ref = nullptr;
};

class Binary {
public:
  constexpr Binary() noexcept = default;
  constexpr Binary(LLVMBinaryRef R) noexcept : Ref(R) {}
  Binary(const Binary &) = delete;
  Binary &operator=(const Binary &) = delete;
  Binary(Binary &&B) noexcept : Binary() { swap(*this, B); }
  Binary &operator=(Binary &&B) noexcept {
    swap(*this, B);
    return *this;
  }

  ~Binary() noexcept { LLVMDisposeBinary(Ref); }

  constexpr operator bool() const noexcept { return Ref != nullptr; }
  constexpr auto &unwrap() const noexcept { return Ref; }
  constexpr auto &unwrap() noexcept { return Ref; }
  friend void swap(Binary &LHS, Binary &RHS) noexcept {
    using std::swap;
    swap(LHS.Ref, RHS.Ref);
  }

  static std::pair<Binary, Message> create(MemoryBuffer &M,
                                           Context &C) noexcept {
    std::pair<Binary, Message> Result;
    Result.first =
        LLVMCreateBinary(M.unwrap(), C.unwrap(), &Result.second.unwrap());
    return Result;
  }

  LLVMBinaryType getType() const noexcept { return LLVMBinaryGetType(Ref); }
  SectionIterator sections() const noexcept {
    return LLVMObjectFileCopySectionIterator(Ref);
  }
  bool isSectionEnd(const SectionIterator &It) const noexcept {
    return LLVMObjectFileIsSectionIteratorAtEnd(Ref, It.unwrap());
  }
  SymbolIterator symbols() const noexcept {
    return LLVMObjectFileCopySymbolIterator(Ref);
  }
  bool isSymbolEnd(const SymbolIterator &It) const noexcept {
    return LLVMObjectFileIsSymbolIteratorAtEnd(Ref, It.unwrap());
  }

private:
  LLVMBinaryRef Ref = nullptr;
};

} // namespace WasmEdge::AOT::LLVM

#include <llvm/IR/GlobalValue.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

namespace WasmEdge::AOT::LLVM {

void Value::setDSOLocal(bool Local) noexcept {
  llvm::cast<llvm::GlobalValue>(reinterpret_cast<llvm::Value *>(Ref))
      ->setDSOLocal(Local);
}

void Value::eliminateUnreachableBlocks() noexcept {
  llvm::EliminateUnreachableBlocks(
      *llvm::cast<llvm::Function>(reinterpret_cast<llvm::Value *>(Ref)));
}

bool SectionIterator::isText() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isText();
}

bool SectionIterator::isData() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isData();
}

bool SectionIterator::isBSS() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isBSS();
}

bool SectionIterator::isPData() const noexcept {
#if WASMEDGE_OS_WINDOWS
  using namespace std::literals;
  return ".pdata"sv == getName();
#else
  return false;
#endif
}

} // namespace WasmEdge::AOT::LLVM
