// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasmedge/wasmedge.h"

#include "common/defines.h"
#include "driver/compiler.h"
#include "driver/tool.h"
#include "driver/unitool.h"
#include "host/wasi/wasimodule.h"
#include "plugin/plugin.h"
#include "system/winapi.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#ifdef WASMEDGE_BUILD_FUZZING
#include "driver/fuzzPO.h"
#include "driver/fuzzTool.h"
#endif

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
#include "driver/wasiNNRPCServerTool.h"
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// WasmEdge_ConfigureContext implementation.
struct WasmEdge_ConfigureContext {
  WasmEdge::Configure Conf;
};

// WasmEdge_StatisticsContext implementation.
struct WasmEdge_StatisticsContext {};

// WasmEdge_ASTModuleContext implementation.
struct WasmEdge_ASTModuleContext {};

// WasmEdge_FunctionTypeContext implementation.
struct WasmEdge_FunctionTypeContext {};

// WasmEdge_TableTypeContext implementation.
struct WasmEdge_TableTypeContext {};

// WasmEdge_MemoryTypeContext implementation.
struct WasmEdge_MemoryTypeContext {};

// WasmEdge_TagTypeContext implementation.
struct WasmEdge_TagTypeContext {};

// WasmEdge_GlobalTypeContext implementation.
struct WasmEdge_GlobalTypeContext {};

// WasmEdge_ImportTypeContext implementation.
struct WasmEdge_ImportTypeContext {};

// WasmEdge_ExportTypeContext implementation.
struct WasmEdge_ExportTypeContext {};

// WasmEdge_CompilerContext implementation.
struct WasmEdge_CompilerContext {
#ifdef WASMEDGE_USE_LLVM
  WasmEdge_CompilerContext(const WasmEdge::Configure &Conf) noexcept
      : Compiler(Conf), CodeGen(Conf), Load(Conf), Valid(Conf) {}
  WasmEdge::LLVM::Compiler Compiler;
  WasmEdge::LLVM::CodeGen CodeGen;
  WasmEdge::Loader::Loader Load;
  WasmEdge::Validator::Validator Valid;
#endif
};

// WasmEdge_LoaderContext implementation.
struct WasmEdge_LoaderContext {};

// WasmEdge_ValidatorContext implementation.
struct WasmEdge_ValidatorContext {};

// WasmEdge_ExecutorContext implementation.
struct WasmEdge_ExecutorContext {};

// WasmEdge_StoreContext implementation.
struct WasmEdge_StoreContext {};

// WasmEdge_ModuleInstanceContext implementation.
struct WasmEdge_ModuleInstanceContext {};

// WasmEdge_FunctionInstanceContext implementation.
struct WasmEdge_FunctionInstanceContext {};

// WasmEdge_TableInstanceContext implementation.
struct WasmEdge_TableInstanceContext {};

// WasmEdge_MemoryInstanceContext implementation.
struct WasmEdge_MemoryInstanceContext {};

// WasmEdge_TagInstanceContext implementation.
struct WasmEdge_TagInstanceContext {};

// WasmEdge_GlobalInstanceContext implementation.
struct WasmEdge_GlobalInstanceContext {};

// WasmEdge_CallingFrameContext implementation.
struct WasmEdge_CallingFrameContext {};

// WasmEdge_Async implementation.
struct WasmEdge_Async {
  template <typename... Args>
  WasmEdge_Async(Args &&...Vals) noexcept
      : Async(std::forward<Args>(Vals)...) {}
  WasmEdge::Async<WasmEdge::Expect<
      std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>>
      Async;
};

// WasmEdge_VMContext implementation.
struct WasmEdge_VMContext {
  template <typename... Args>
  WasmEdge_VMContext(Args &&...Vals) noexcept
      : VM(std::forward<Args>(Vals)...) {}
  WasmEdge::VM::VM VM;
};

// WasmEdge_PluginContext implementation.
struct WasmEdge_PluginContext {};

namespace {

using namespace WasmEdge;

// Helper function for returning a WasmEdge_Result by error code.
inline constexpr WasmEdge_Result
genWasmEdge_Result(const ErrCode::Value &Code) noexcept {
  return WasmEdge_Result{/* Code */ static_cast<uint32_t>(Code) & 0x00FFFFFFU};
}
inline constexpr WasmEdge_Result
genWasmEdge_Result(const ErrCode &Code) noexcept {
  return WasmEdge_Result{/* Code */ Code.operator uint32_t()};
}

// Helper function for returning a struct uint128_t / int128_t
// from class WasmEdge::uint128_t / WasmEdge::int128_t.
template <typename C>
inline constexpr ::uint128_t to_uint128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return {/* Low */ Val.low(), /* High */ static_cast<uint64_t>(Val.high())};
#endif
}
template <typename C> inline constexpr ::int128_t to_int128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return {/* Low */ Val.low(), /* High */ Val.high()};
#endif
}

// Helper function for returning a class WasmEdge::uint128_t /
// WasmEdge::int128_t from struct uint128_t / int128_t.
template <typename C, typename T>
inline constexpr C to_WasmEdge_128_t(T Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return C(Val.High, Val.Low);
#endif
}

// Helper functions for returning a WasmEdge::ValType by WasmEdge_ValType.
inline ValType genValType(const WasmEdge_ValType &T) noexcept {
  std::array<uint8_t, 8> R;
  std::copy_n(T.Data, 8, R.begin());
  return ValType(R);
}

// Helper functions for returning a WasmEdge_ValType by WasmEdge::ValType.
inline WasmEdge_ValType genWasmEdge_ValType(const ValType &T) noexcept {
  WasmEdge_ValType VT;
  std::copy_n(T.getRawData().cbegin(), 8, VT.Data);
  return VT;
}

// Helper functions for returning a WasmEdge_Value by various values.
template <typename T>
inline WasmEdge_Value genWasmEdge_Value(const T &Val) noexcept {
  return WasmEdge_Value{
      /* Value */ to_uint128_t(ValVariant(Val).unwrap()),
      /* Type */ genWasmEdge_ValType(WasmEdge::ValTypeFromType<T>())};
}
inline WasmEdge_Value genWasmEdge_Value(const ValVariant &Val,
                                        const ValType &T) noexcept {
  return WasmEdge_Value{/* Value */ to_uint128_t(Val.unwrap()),
                        /* Type */ genWasmEdge_ValType(T)};
}

// Helper function for converting a WasmEdge_Value array to a ValVariant
// vector.
inline std::pair<std::vector<ValVariant>, std::vector<ValType>>
genParamPair(const WasmEdge_Value *Val, const uint32_t Len) noexcept {
  // The nullable value in reference types checking is handled in executor.
  std::vector<ValVariant> VVec;
  std::vector<ValType> TVec;
  if (Val == nullptr) {
    return {VVec, TVec};
  }
  VVec.resize(Len);
  TVec.resize(Len);
  for (uint32_t I = 0; I < Len; I++) {
    TVec[I] = genValType(Val[I].Type);
    switch (TVec[I].getCode()) {
    case TypeCode::I32:
      VVec[I] = ValVariant::wrap<uint32_t>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case TypeCode::I64:
      VVec[I] = ValVariant::wrap<uint64_t>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case TypeCode::F32:
      VVec[I] = ValVariant::wrap<float>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case TypeCode::F64:
      VVec[I] = ValVariant::wrap<double>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case TypeCode::V128:
      VVec[I] = ValVariant::wrap<WasmEdge::uint128_t>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case TypeCode::Ref:
    case TypeCode::RefNull: {
      VVec[I] = ValVariant::wrap<RefVariant>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {VVec, TVec};
}

// Helper function for making a Span to a uint8_t array.
template <typename T>
inline constexpr Span<const T> genSpan(const T *Buf,
                                       const uint32_t Len) noexcept {
  if (Buf && Len > 0) {
    return Span<const T>(Buf, Len);
  }
  return Span<const T>();
}

// Helper functions for converting WasmEdge_String to std::String.
inline std::string_view genStrView(const WasmEdge_String S) noexcept {
  return std::string_view(S.Buf, S.Length);
}

// Helper functions for converting a ValVariant vector to a WasmEdge_Value
// array.
inline constexpr void
fillWasmEdge_ValueArr(Span<const std::pair<ValVariant, ValType>> Vec,
                      WasmEdge_Value *Val, const uint32_t Len) noexcept {
  if (Val == nullptr) {
    return;
  }
  for (uint32_t I = 0; I < Len && I < Vec.size(); I++) {
    Val[I] = genWasmEdge_Value(Vec[I].first, Vec[I].second);
  }
}

// Helper template to run and return result.
auto EmptyThen = [](auto &&) noexcept {};
template <typename T> inline bool isContext(T *Cxt) noexcept {
  return (Cxt != nullptr);
}
template <typename T, typename... Args>
inline bool isContext(T *Cxt, Args *...Cxts) noexcept {
  return isContext(Cxt) && isContext(Cxts...);
}
template <typename T, typename U, typename... CxtT>
inline WasmEdge_Result wrap(T &&Proc, U &&Then, CxtT *...Cxts) noexcept {
  if (isContext(Cxts...)) {
    if (auto Res = Proc()) {
      Then(Res);
      return genWasmEdge_Result(ErrCode::Value::Success);
    } else {
      return genWasmEdge_Result(Res.error());
    }
  } else {
    return genWasmEdge_Result(ErrCode::Value::WrongVMWorkflow);
  }
}

// Helper function of retrieving exported maps.
template <typename T>
inline uint32_t fillMap(const std::map<std::string, T *, std::less<>> &Map,
                        WasmEdge_String *Names, const uint32_t Len) noexcept {
  uint32_t I = 0;
  for (auto &&Pair : Map) {
    if (I >= Len) {
      break;
    }
    if (Names) {
      Names[I] = WasmEdge_String{
          /* Length */ static_cast<uint32_t>(Pair.first.length()),
          /* Buf */ Pair.first.data()};
    }
    I++;
  }
  return static_cast<uint32_t>(Map.size());
}

// Helper functions of context conversions.
#define CONVTO(SIMP, INST, NAME, QUANT)                                        \
  inline QUANT auto *to##SIMP##Cxt(QUANT INST *Cxt) noexcept {                 \
    return reinterpret_cast<QUANT WasmEdge_##NAME##Context *>(Cxt);            \
  }
CONVTO(Stat, Statistics::Statistics, Statistics, )
CONVTO(ASTMod, AST::Module, ASTModule, )
CONVTO(FuncType, AST::FunctionType, FunctionType, )
CONVTO(FuncType, AST::FunctionType, FunctionType, const)
CONVTO(TabType, AST::TableType, TableType, )
CONVTO(TabType, AST::TableType, TableType, const)
CONVTO(MemType, AST::MemoryType, MemoryType, )
CONVTO(MemType, AST::MemoryType, MemoryType, const)
CONVTO(TagType, AST::TagType, TagType, const)
CONVTO(GlobType, AST::GlobalType, GlobalType, )
CONVTO(GlobType, AST::GlobalType, GlobalType, const)
CONVTO(ImpType, AST::ImportDesc, ImportType, const)
CONVTO(ExpType, AST::ExportDesc, ExportType, const)
CONVTO(Store, Runtime::StoreManager, Store, )
CONVTO(Loader, Loader::Loader, Loader, )
CONVTO(Validator, Validator::Validator, Validator, )
CONVTO(Executor, Executor::Executor, Executor, )
CONVTO(Mod, Runtime::Instance::ModuleInstance, ModuleInstance, )
CONVTO(Mod, Runtime::Instance::ModuleInstance, ModuleInstance, const)
CONVTO(Func, Runtime::Instance::FunctionInstance, FunctionInstance, )
CONVTO(Func, Runtime::Instance::FunctionInstance, FunctionInstance, const)
CONVTO(Tab, Runtime::Instance::TableInstance, TableInstance, )
CONVTO(Mem, Runtime::Instance::MemoryInstance, MemoryInstance, )
CONVTO(Tag, Runtime::Instance::TagInstance, TagInstance, )
CONVTO(Glob, Runtime::Instance::GlobalInstance, GlobalInstance, )
CONVTO(CallFrame, Runtime::CallingFrame, CallingFrame, const)
CONVTO(Plugin, Plugin::Plugin, Plugin, const)
#undef CONVTO

#define CONVFROM(SIMP, INST, NAME, QUANT)                                      \
  inline QUANT auto *from##SIMP##Cxt(                                          \
      QUANT WasmEdge_##NAME##Context *Cxt) noexcept {                          \
    return reinterpret_cast<QUANT INST *>(Cxt);                                \
  }
CONVFROM(Stat, Statistics::Statistics, Statistics, )
CONVFROM(Stat, Statistics::Statistics, Statistics, const)
CONVFROM(ASTMod, AST::Module, ASTModule, )
CONVFROM(ASTMod, AST::Module, ASTModule, const)
CONVFROM(FuncType, AST::FunctionType, FunctionType, )
CONVFROM(FuncType, AST::FunctionType, FunctionType, const)
CONVFROM(TabType, AST::TableType, TableType, )
CONVFROM(TabType, AST::TableType, TableType, const)
CONVFROM(MemType, AST::MemoryType, MemoryType, )
CONVFROM(MemType, AST::MemoryType, MemoryType, const)
CONVFROM(TagType, AST::TagType, TagType, const)
CONVFROM(GlobType, AST::GlobalType, GlobalType, )
CONVFROM(GlobType, AST::GlobalType, GlobalType, const)
CONVFROM(ImpType, AST::ImportDesc, ImportType, const)
CONVFROM(ExpType, AST::ExportDesc, ExportType, const)
CONVFROM(Store, Runtime::StoreManager, Store, )
CONVFROM(Store, Runtime::StoreManager, Store, const)
CONVFROM(Loader, Loader::Loader, Loader, )
CONVFROM(Validator, Validator::Validator, Validator, )
CONVFROM(Executor, Executor::Executor, Executor, )
CONVFROM(Mod, Runtime::Instance::ModuleInstance, ModuleInstance, )
CONVFROM(Mod, Runtime::Instance::ModuleInstance, ModuleInstance, const)
CONVFROM(Func, Runtime::Instance::FunctionInstance, FunctionInstance, )
CONVFROM(Func, Runtime::Instance::FunctionInstance, FunctionInstance, const)
CONVFROM(Tab, Runtime::Instance::TableInstance, TableInstance, )
CONVFROM(Tab, Runtime::Instance::TableInstance, TableInstance, const)
CONVFROM(Mem, Runtime::Instance::MemoryInstance, MemoryInstance, )
CONVFROM(Mem, Runtime::Instance::MemoryInstance, MemoryInstance, const)
CONVFROM(Tag, Runtime::Instance::TagInstance, TagInstance, const)
CONVFROM(Glob, Runtime::Instance::GlobalInstance, GlobalInstance, )
CONVFROM(Glob, Runtime::Instance::GlobalInstance, GlobalInstance, const)
CONVFROM(CallFrame, Runtime::CallingFrame, CallingFrame, const)
CONVFROM(Plugin, Plugin::Plugin, Plugin, const)
#undef CONVFROM

// C API Host function class
class CAPIHostFunc : public Runtime::HostFunctionBase {
public:
  CAPIHostFunc(const AST::FunctionType *Type, WasmEdge_HostFunc_t FuncPtr,
               void *ExtData, const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(FuncPtr), Wrap(nullptr),
        Binding(nullptr), Data(ExtData) {
    DefType.getCompositeType().getFuncType() = *Type;
  }
  CAPIHostFunc(const AST::FunctionType *Type, WasmEdge_WrapFunc_t WrapPtr,
               void *BindingPtr, void *ExtData,
               const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(nullptr), Wrap(WrapPtr),
        Binding(BindingPtr), Data(ExtData) {
    DefType.getCompositeType().getFuncType() = *Type;
  }
  ~CAPIHostFunc() noexcept override = default;

  Expect<void> run(const Runtime::CallingFrame &CallFrame,
                   Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    auto &FuncType = DefType.getCompositeType().getFuncType();
    std::vector<WasmEdge_Value> Params(FuncType.getParamTypes().size()),
        Returns(FuncType.getReturnTypes().size());
    for (uint32_t I = 0; I < Args.size(); I++) {
      Params[I] = genWasmEdge_Value(Args[I], FuncType.getParamTypes()[I]);
    }
    WasmEdge_Value *PPtr = Params.size() ? (&Params[0]) : nullptr;
    WasmEdge_Value *RPtr = Returns.size() ? (&Returns[0]) : nullptr;
    auto *CallFrameCxt = toCallFrameCxt(&CallFrame);
    WasmEdge_Result Stat;
    if (Func) {
      Stat = Func(Data, CallFrameCxt, PPtr, RPtr);
    } else {
      Stat = Wrap(Binding, Data, CallFrameCxt, PPtr,
                  static_cast<uint32_t>(Params.size()), RPtr,
                  static_cast<uint32_t>(Returns.size()));
    }
    for (uint32_t I = 0; I < Rets.size(); I++) {
      Rets[I] = to_WasmEdge_128_t<WasmEdge::uint128_t>(Returns[I].Value);
    }
    if (WasmEdge_ResultOK(Stat)) {
      if (WasmEdge_ResultGetCode(Stat) == 0x01U) {
        return Unexpect(ErrCode::Value::Terminated);
      }
    } else {
      return Unexpect(
          static_cast<ErrCategory>(WasmEdge_ResultGetCategory(Stat)),
          WasmEdge_ResultGetCode(Stat));
    }
    return {};
  }
  void *getData() const noexcept { return Data; }

private:
  WasmEdge_HostFunc_t Func;
  WasmEdge_WrapFunc_t Wrap;
  void *Binding;
  void *Data;
};

} // namespace

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge version functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT const char *WasmEdge_VersionGet(void) {
  return WASMEDGE_VERSION;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_VersionGetMajor(void) {
  return WASMEDGE_VERSION_MAJOR;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_VersionGetMinor(void) {
  return WASMEDGE_VERSION_MINOR;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_VersionGetPatch(void) {
  return WASMEDGE_VERSION_PATCH;
}

// <<<<<<<< WasmEdge version functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge logging functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT void WasmEdge_LogSetErrorLevel(void) {
  WasmEdge::Log::setErrorLoggingLevel();
}

WASMEDGE_CAPI_EXPORT void WasmEdge_LogSetDebugLevel(void) {
  WasmEdge::Log::setDebugLoggingLevel();
}

WASMEDGE_CAPI_EXPORT void WasmEdge_LogOff(void) { WasmEdge::Log::setLogOff(); }

// <<<<<<<< WasmEdge logging functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge valtype functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenI32(void) {
  return genWasmEdge_ValType(ValType(TypeCode::I32));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenI64(void) {
  return genWasmEdge_ValType(ValType(TypeCode::I64));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenF32(void) {
  return genWasmEdge_ValType(ValType(TypeCode::F32));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenF64(void) {
  return genWasmEdge_ValType(ValType(TypeCode::F64));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenV128(void) {
  return genWasmEdge_ValType(ValType(TypeCode::V128));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenFuncRef(void) {
  return genWasmEdge_ValType(ValType(TypeCode::FuncRef));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType WasmEdge_ValTypeGenExternRef(void) {
  return genWasmEdge_ValType(ValType(TypeCode::ExternRef));
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsEqual(const WasmEdge_ValType ValType1,
                        const WasmEdge_ValType ValType2) {
  return genValType(ValType1) == genValType(ValType2);
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsI32(const WasmEdge_ValType ValType) {
  return genValType(ValType).getCode() == WasmEdge::TypeCode::I32;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsI64(const WasmEdge_ValType ValType) {
  return genValType(ValType).getCode() == WasmEdge::TypeCode::I64;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsF32(const WasmEdge_ValType ValType) {
  return genValType(ValType).getCode() == WasmEdge::TypeCode::F32;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsF64(const WasmEdge_ValType ValType) {
  return genValType(ValType).getCode() == WasmEdge::TypeCode::F64;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsV128(const WasmEdge_ValType ValType) {
  return genValType(ValType).getCode() == WasmEdge::TypeCode::V128;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsFuncRef(const WasmEdge_ValType ValType) {
  return genValType(ValType).isFuncRefType();
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsExternRef(const WasmEdge_ValType ValType) {
  return genValType(ValType).isExternRefType();
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsRef(const WasmEdge_ValType ValType) {
  return genValType(ValType).isRefType();
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ValTypeIsRefNull(const WasmEdge_ValType ValType) {
  return genValType(ValType).isNullableRefType();
}

// <<<<<<<< WasmEdge valtype functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge value functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenI32(const int32_t Val) {
  return genWasmEdge_Value(Val);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenI64(const int64_t Val) {
  return genWasmEdge_Value(Val);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenF32(const float Val) {
  return genWasmEdge_Value(Val);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenF64(const double Val) {
  return genWasmEdge_Value(Val);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value
WasmEdge_ValueGenV128(const ::int128_t Val) {
  return genWasmEdge_Value(to_WasmEdge_128_t<WasmEdge::int128_t>(Val));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value
WasmEdge_ValueGenFuncRef(const WasmEdge_FunctionInstanceContext *Cxt) {
  return genWasmEdge_Value(WasmEdge::RefVariant(fromFuncCxt(Cxt)),
                           TypeCode::FuncRef);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenExternRef(void *Ref) {
  return genWasmEdge_Value(WasmEdge::RefVariant(Ref), TypeCode::ExternRef);
}

WASMEDGE_CAPI_EXPORT int32_t WasmEdge_ValueGetI32(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<int32_t>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
      .get<int32_t>();
}

WASMEDGE_CAPI_EXPORT int64_t WasmEdge_ValueGetI64(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<int64_t>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
      .get<int64_t>();
}

WASMEDGE_CAPI_EXPORT float WasmEdge_ValueGetF32(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<float>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
      .get<float>();
}

WASMEDGE_CAPI_EXPORT double WasmEdge_ValueGetF64(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<double>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
      .get<double>();
}

WASMEDGE_CAPI_EXPORT ::int128_t
WasmEdge_ValueGetV128(const WasmEdge_Value Val) {
  return to_int128_t(WasmEdge::ValVariant::wrap<WasmEdge::int128_t>(
                         to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
                         .get<WasmEdge::int128_t>());
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ValueIsNullRef(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<WasmEdge::RefVariant>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
      .get<WasmEdge::RefVariant>()
      .isNull();
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionInstanceContext *
WasmEdge_ValueGetFuncRef(const WasmEdge_Value Val) {
  return toFuncCxt(WasmEdge::retrieveFuncRef(
      WasmEdge::ValVariant::wrap<WasmEdge::RefVariant>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
          .get<WasmEdge::RefVariant>()
          .getPtr<WasmEdge::Runtime::Instance::FunctionInstance>()));
}

WASMEDGE_CAPI_EXPORT void *
WasmEdge_ValueGetExternRef(const WasmEdge_Value Val) {
  return &WasmEdge::retrieveExternRef<uint32_t>(
      WasmEdge::ValVariant::wrap<WasmEdge::RefVariant>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value))
          .get<WasmEdge::RefVariant>());
}

// <<<<<<<< WasmEdge value functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge string functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_StringCreateByCString(const char *Str) {
  if (Str) {
    return WasmEdge_StringCreateByBuffer(
        Str, static_cast<uint32_t>(std::strlen(Str)));
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_StringCreateByBuffer(const char *Buf, const uint32_t Len) {
  if (Buf && Len) {
    char *Str = new char[Len];
    std::copy_n(Buf, Len, Str);
    return WasmEdge_String{/* Length */ Len, /* Buf */ Str};
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_String WasmEdge_StringWrap(const char *Buf,
                                                         const uint32_t Len) {
  return WasmEdge_String{/* Length */ Len, /* Buf */ Buf};
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_StringIsEqual(const WasmEdge_String Str1,
                                                 const WasmEdge_String Str2) {
  if (Str1.Length != Str2.Length) {
    return false;
  }
  return std::equal(Str1.Buf, Str1.Buf + Str1.Length, Str2.Buf);
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StringCopy(const WasmEdge_String Str,
                                                  char *Buf,
                                                  const uint32_t Len) {
  if (Buf) {
    std::memset(Buf, 0, Len);
  }
  uint32_t RealLength = std::min(Len, Str.Length);
  if (RealLength > 0) {
    std::copy_n(Str.Buf, RealLength, Buf);
  }
  return RealLength;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_StringDelete(WasmEdge_String Str) {
  if (Str.Buf) {
    delete[] Str.Buf;
  }
}

// <<<<<<<< WasmEdge string functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge bytes functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_Bytes WasmEdge_BytesCreate(const uint8_t *Buf,
                                                         const uint32_t Len) {
  if (Buf && Len) {
    uint8_t *Str = new uint8_t[Len];
    std::copy_n(Buf, Len, Str);
    return WasmEdge_Bytes{/* Length */ Len, /* Buf */ Str};
  }
  return WasmEdge_Bytes{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_Bytes WasmEdge_BytesWrap(const uint8_t *Buf,
                                                       const uint32_t Len) {
  return WasmEdge_Bytes{/* Length */ Len, /* Buf */ Buf};
}

WASMEDGE_CAPI_EXPORT void WasmEdge_BytesDelete(WasmEdge_Bytes Bytes) {
  if (Bytes.Buf) {
    delete[] Bytes.Buf;
  }
}

// <<<<<<<< WasmEdge bytes functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge result functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT bool WasmEdge_ResultOK(const WasmEdge_Result Res) {
  if (WasmEdge_ResultGetCategory(Res) == WasmEdge_ErrCategory_WASM &&
      (static_cast<WasmEdge::ErrCode::Value>(WasmEdge_ResultGetCode(Res)) ==
           WasmEdge::ErrCode::Value::Success ||
       static_cast<WasmEdge::ErrCode::Value>(WasmEdge_ResultGetCode(Res)) ==
           WasmEdge::ErrCode::Value::Terminated)) {
    return true;
  } else {
    return false;
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ResultGen(
    const enum WasmEdge_ErrCategory Category, const uint32_t Code) {
  return WasmEdge_Result{/* Code */ (static_cast<uint32_t>(Category) << 24) +
                         (Code & 0x00FFFFFFU)};
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ResultGetCode(const WasmEdge_Result Res) {
  return Res.Code & 0x00FFFFFFU;
}

WASMEDGE_CAPI_EXPORT WasmEdge_ErrCategory
WasmEdge_ResultGetCategory(const WasmEdge_Result Res) {
  return static_cast<WasmEdge_ErrCategory>(Res.Code >> 24);
}

WASMEDGE_CAPI_EXPORT const char *
WasmEdge_ResultGetMessage(const WasmEdge_Result Res) {
  if (WasmEdge_ResultGetCategory(Res) != WasmEdge_ErrCategory_WASM) {
    return WasmEdge::ErrCodeStr[WasmEdge::ErrCode::Value::UserDefError].data();
  }
  return WasmEdge::ErrCodeStr[static_cast<WasmEdge::ErrCode::Value>(
                                  WasmEdge_ResultGetCode(Res))]
      .data();
}

// <<<<<<<< WasmEdge result functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge limit functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT bool WasmEdge_LimitIsEqual(const WasmEdge_Limit Lim1,
                                                const WasmEdge_Limit Lim2) {
  return Lim1.HasMax == Lim2.HasMax && Lim1.Shared == Lim2.Shared &&
         Lim1.Min == Lim2.Min && Lim1.Max == Lim2.Max;
}

// <<<<<<<< WasmEdge limit functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge configure functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ConfigureContext *WasmEdge_ConfigureCreate(void) {
  return new WasmEdge_ConfigureContext;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureAddProposal(WasmEdge_ConfigureContext *Cxt,
                              const enum WasmEdge_Proposal Prop) {
  if (Cxt) {
    Cxt->Conf.addProposal(static_cast<WasmEdge::Proposal>(Prop));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureRemoveProposal(WasmEdge_ConfigureContext *Cxt,
                                 const enum WasmEdge_Proposal Prop) {
  if (Cxt) {
    Cxt->Conf.removeProposal(static_cast<WasmEdge::Proposal>(Prop));
  }
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ConfigureHasProposal(const WasmEdge_ConfigureContext *Cxt,
                              const enum WasmEdge_Proposal Prop) {
  if (Cxt) {
    return Cxt->Conf.hasProposal(static_cast<WasmEdge::Proposal>(Prop));
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ConfigureAddHostRegistration(
    WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host) {
  if (Cxt) {
    Cxt->Conf.addHostRegistration(
        static_cast<WasmEdge::HostRegistration>(Host));
  }
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ConfigureRemoveHostRegistration(
    WasmEdge_ConfigureContext *Cxt, const enum WasmEdge_HostRegistration Host) {
  if (Cxt) {
    Cxt->Conf.removeHostRegistration(
        static_cast<WasmEdge::HostRegistration>(Host));
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureHasHostRegistration(
    const WasmEdge_ConfigureContext *Cxt,
    const enum WasmEdge_HostRegistration Host) {
  if (Cxt) {
    return Cxt->Conf.hasHostRegistration(
        static_cast<WasmEdge::HostRegistration>(Host));
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureSetMaxMemoryPage(WasmEdge_ConfigureContext *Cxt,
                                   const uint32_t Page) {
  if (Cxt) {
    Cxt->Conf.getRuntimeConfigure().setMaxMemoryPage(Page);
  }
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ConfigureGetMaxMemoryPage(const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getRuntimeConfigure().getMaxMemoryPage();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureSetForceInterpreter(WasmEdge_ConfigureContext *Cxt,
                                      const bool IsForceInterpreter) {
  if (Cxt) {
    Cxt->Conf.getRuntimeConfigure().setForceInterpreter(IsForceInterpreter);
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureSetAllowAFUNIX(WasmEdge_ConfigureContext *Cxt,
                                 const bool EnableAFUNIX) {
  if (Cxt) {
    Cxt->Conf.getRuntimeConfigure().setAllowAFUNIX(EnableAFUNIX);
  }
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ConfigureIsAllowAFUNIX(const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getRuntimeConfigure().isAllowAFUNIX();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ConfigureIsForceInterpreter(const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getRuntimeConfigure().isForceInterpreter();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ConfigureCompilerSetOptimizationLevel(
    WasmEdge_ConfigureContext *Cxt,
    const enum WasmEdge_CompilerOptimizationLevel Level) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setOptimizationLevel(
        static_cast<WasmEdge::CompilerConfigure::OptimizationLevel>(Level));
  }
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_CompilerOptimizationLevel
WasmEdge_ConfigureCompilerGetOptimizationLevel(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_CompilerOptimizationLevel>(
        Cxt->Conf.getCompilerConfigure().getOptimizationLevel());
  }
  return WasmEdge_CompilerOptimizationLevel_O0;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ConfigureCompilerSetOutputFormat(
    WasmEdge_ConfigureContext *Cxt,
    const enum WasmEdge_CompilerOutputFormat Format) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setOutputFormat(
        static_cast<WasmEdge::CompilerConfigure::OutputFormat>(Format));
  }
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_CompilerOutputFormat
WasmEdge_ConfigureCompilerGetOutputFormat(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_CompilerOutputFormat>(
        Cxt->Conf.getCompilerConfigure().getOutputFormat());
  }
  return WasmEdge_CompilerOutputFormat_Wasm;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureCompilerSetDumpIR(WasmEdge_ConfigureContext *Cxt,
                                    const bool IsDump) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setDumpIR(IsDump);
  }
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ConfigureCompilerIsDumpIR(const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getCompilerConfigure().isDumpIR();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureCompilerSetGenericBinary(WasmEdge_ConfigureContext *Cxt,
                                           const bool IsGeneric) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setGenericBinary(IsGeneric);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureCompilerIsGenericBinary(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getCompilerConfigure().isGenericBinary();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureCompilerSetInterruptible(WasmEdge_ConfigureContext *Cxt,
                                           const bool IsInterruptible) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setInterruptible(IsInterruptible);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureCompilerIsInterruptible(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getCompilerConfigure().isInterruptible();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ConfigureStatisticsSetInstructionCounting(
    WasmEdge_ConfigureContext *Cxt, const bool IsCount) {
  if (Cxt) {
    Cxt->Conf.getStatisticsConfigure().setInstructionCounting(IsCount);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureStatisticsIsInstructionCounting(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getStatisticsConfigure().isInstructionCounting();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureStatisticsSetCostMeasuring(WasmEdge_ConfigureContext *Cxt,
                                             const bool IsMeasure) {
  if (Cxt) {
    Cxt->Conf.getStatisticsConfigure().setCostMeasuring(IsMeasure);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureStatisticsIsCostMeasuring(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getStatisticsConfigure().isCostMeasuring();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureStatisticsSetTimeMeasuring(WasmEdge_ConfigureContext *Cxt,
                                             const bool IsMeasure) {
  if (Cxt) {
    Cxt->Conf.getStatisticsConfigure().setTimeMeasuring(IsMeasure);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureStatisticsIsTimeMeasuring(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getStatisticsConfigure().isTimeMeasuring();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureDelete(WasmEdge_ConfigureContext *Cxt) {
  delete Cxt;
}

// <<<<<<<< WasmEdge configure functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// TODO Add C_API support for coredump
// WASMEDGE_CAPI_EXPORT WasmEdge_CoredumpContext *
//    WasmEdge_CoredumpCreate(void) {
//  return toCoredumpCxt(new WasmEdge::Coredump::Coredump);
//}

// >>>>>>>> WasmEdge statistics functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_StatisticsContext *
WasmEdge_StatisticsCreate(void) {
  return toStatCxt(new WasmEdge::Statistics::Statistics);
}

WASMEDGE_CAPI_EXPORT uint64_t
WasmEdge_StatisticsGetInstrCount(const WasmEdge_StatisticsContext *Cxt) {
  if (Cxt) {
    return fromStatCxt(Cxt)->getInstrCount();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT double
WasmEdge_StatisticsGetInstrPerSecond(const WasmEdge_StatisticsContext *Cxt) {
  if (Cxt) {
    return fromStatCxt(Cxt)->getInstrPerSecond();
  }
  return 0.0;
}

WASMEDGE_CAPI_EXPORT uint64_t
WasmEdge_StatisticsGetTotalCost(const WasmEdge_StatisticsContext *Cxt) {
  if (Cxt) {
    return fromStatCxt(Cxt)->getTotalCost();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_StatisticsSetCostTable(WasmEdge_StatisticsContext *Cxt,
                                uint64_t *CostArr, const uint32_t Len) {
  if (Cxt) {
    fromStatCxt(Cxt)->setCostTable(genSpan(CostArr, Len));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_StatisticsSetCostLimit(WasmEdge_StatisticsContext *Cxt,
                                const uint64_t Limit) {
  if (Cxt) {
    fromStatCxt(Cxt)->setCostLimit(Limit);
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_StatisticsClear(WasmEdge_StatisticsContext *Cxt) {
  if (Cxt) {
    fromStatCxt(Cxt)->clear();
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_StatisticsDelete(WasmEdge_StatisticsContext *Cxt) {
  delete fromStatCxt(Cxt);
}

// <<<<<<<< WasmEdge statistics functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge AST module functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ASTModuleListImportsLength(const WasmEdge_ASTModuleContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(
        fromASTModCxt(Cxt)->getImportSection().getContent().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ASTModuleListImports(
    const WasmEdge_ASTModuleContext *Cxt,
    const WasmEdge_ImportTypeContext **Imports, const uint32_t Len) {
  if (Cxt) {
    const auto &ImpSec = fromASTModCxt(Cxt)->getImportSection().getContent();
    if (Imports) {
      for (uint32_t I = 0; I < Len && I < ImpSec.size(); I++) {
        Imports[I] = toImpTypeCxt(&ImpSec[I]);
      }
    }
    return static_cast<uint32_t>(ImpSec.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ASTModuleListExportsLength(const WasmEdge_ASTModuleContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(
        fromASTModCxt(Cxt)->getExportSection().getContent().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ASTModuleListExports(
    const WasmEdge_ASTModuleContext *Cxt,
    const WasmEdge_ExportTypeContext **Exports, const uint32_t Len) {
  if (Cxt) {
    const auto &ExpSec = fromASTModCxt(Cxt)->getExportSection().getContent();
    if (Exports) {
      for (uint32_t I = 0; I < Len && I < ExpSec.size(); I++) {
        Exports[I] = toExpTypeCxt(&ExpSec[I]);
      }
    }
    return static_cast<uint32_t>(ExpSec.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ASTModuleDelete(WasmEdge_ASTModuleContext *Cxt) {
  std::unique_ptr<WasmEdge::AST::Module> Own(fromASTModCxt(Cxt));
}

// <<<<<<<< WasmEdge AST module functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge function type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionTypeContext *WasmEdge_FunctionTypeCreate(
    const WasmEdge_ValType *ParamList, const uint32_t ParamLen,
    const WasmEdge_ValType *ReturnList, const uint32_t ReturnLen) {
  auto *Cxt = new WasmEdge::AST::FunctionType;
  if (ParamLen > 0) {
    Cxt->getParamTypes().resize(ParamLen);
  }
  for (uint32_t I = 0; I < ParamLen; I++) {
    Cxt->getParamTypes()[I] = genValType(ParamList[I]);
  }
  if (ReturnLen > 0) {
    Cxt->getReturnTypes().resize(ReturnLen);
  }
  for (uint32_t I = 0; I < ReturnLen; I++) {
    Cxt->getReturnTypes()[I] = genValType(ReturnList[I]);
  }
  return toFuncTypeCxt(Cxt);
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_FunctionTypeGetParametersLength(
    const WasmEdge_FunctionTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(fromFuncTypeCxt(Cxt)->getParamTypes().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_FunctionTypeGetParameters(const WasmEdge_FunctionTypeContext *Cxt,
                                   WasmEdge_ValType *List, const uint32_t Len) {
  if (Cxt) {
    for (uint32_t I = 0;
         I < fromFuncTypeCxt(Cxt)->getParamTypes().size() && I < Len; I++) {
      List[I] = genWasmEdge_ValType(fromFuncTypeCxt(Cxt)->getParamTypes()[I]);
    }
    return static_cast<uint32_t>(fromFuncTypeCxt(Cxt)->getParamTypes().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_FunctionTypeGetReturnsLength(const WasmEdge_FunctionTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(fromFuncTypeCxt(Cxt)->getReturnTypes().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_FunctionTypeGetReturns(const WasmEdge_FunctionTypeContext *Cxt,
                                WasmEdge_ValType *List, const uint32_t Len) {
  if (Cxt) {
    for (uint32_t I = 0;
         I < fromFuncTypeCxt(Cxt)->getReturnTypes().size() && I < Len; I++) {
      List[I] = genWasmEdge_ValType(fromFuncTypeCxt(Cxt)->getReturnTypes()[I]);
    }
    return static_cast<uint32_t>(fromFuncTypeCxt(Cxt)->getReturnTypes().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_FunctionTypeDelete(WasmEdge_FunctionTypeContext *Cxt) {
  delete fromFuncTypeCxt(Cxt);
}

// <<<<<<<< WasmEdge function type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge table type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_TableTypeContext *
WasmEdge_TableTypeCreate(const WasmEdge_ValType RefType,
                         const WasmEdge_Limit Limit) {
  WasmEdge::ValType RT = genValType(RefType);
  if (!RT.isRefType()) {
    return nullptr;
  }
  if (Limit.HasMax) {
    return toTabTypeCxt(new WasmEdge::AST::TableType(RT, Limit.Min, Limit.Max));
  } else {
    return toTabTypeCxt(new WasmEdge::AST::TableType(RT, Limit.Min));
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType
WasmEdge_TableTypeGetRefType(const WasmEdge_TableTypeContext *Cxt) {
  if (Cxt) {
    return genWasmEdge_ValType(fromTabTypeCxt(Cxt)->getRefType());
  }
  return WasmEdge_ValTypeGenFuncRef();
}

WASMEDGE_CAPI_EXPORT WasmEdge_Limit
WasmEdge_TableTypeGetLimit(const WasmEdge_TableTypeContext *Cxt) {
  if (Cxt) {
    const auto &Lim = fromTabTypeCxt(Cxt)->getLimit();
    return WasmEdge_Limit{/* HasMax */ Lim.hasMax(),
                          /* Shared */ Lim.isShared(),
                          /* Min */ Lim.getMin(),
                          /* Max */ Lim.getMax()};
  }
  return WasmEdge_Limit{/* HasMax */ false, /* Shared */ false, /* Min */ 0,
                        /* Max */ 0};
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_TableTypeDelete(WasmEdge_TableTypeContext *Cxt) {
  delete fromTabTypeCxt(Cxt);
}

// <<<<<<<< WasmEdge table type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge memory type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryTypeContext *
WasmEdge_MemoryTypeCreate(const WasmEdge_Limit Limit) {
  if (Limit.Shared) {
    return toMemTypeCxt(
        new WasmEdge::AST::MemoryType(Limit.Min, Limit.Max, true));
  } else if (Limit.HasMax) {
    return toMemTypeCxt(new WasmEdge::AST::MemoryType(Limit.Min, Limit.Max));
  } else {
    return toMemTypeCxt(new WasmEdge::AST::MemoryType(Limit.Min));
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Limit
WasmEdge_MemoryTypeGetLimit(const WasmEdge_MemoryTypeContext *Cxt) {
  if (Cxt) {
    const auto &Lim = fromMemTypeCxt(Cxt)->getLimit();
    return WasmEdge_Limit{/* HasMax */ Lim.hasMax(),
                          /* Shared */ Lim.isShared(),
                          /* Min */ Lim.getMin(),
                          /* Max */ Lim.getMax()};
  }
  return WasmEdge_Limit{/* HasMax */ false, /* Shared */ false, /* Min */ 0,
                        /* Max */ 0};
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_MemoryTypeDelete(WasmEdge_MemoryTypeContext *Cxt) {
  delete fromMemTypeCxt(Cxt);
}

// <<<<<<<< WasmEdge memory type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge tag type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_TagTypeGetFunctionType(const WasmEdge_TagTypeContext *Cxt) {
  if (Cxt) {
    const auto &CompType = fromTagTypeCxt(Cxt)->getDefType().getCompositeType();
    if (CompType.isFunc()) {
      return toFuncTypeCxt(&CompType.getFuncType());
    }
  }
  return nullptr;
}

// <<<<<<<< WasmEdge tag type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge global type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalTypeContext *
WasmEdge_GlobalTypeCreate(const WasmEdge_ValType ValType,
                          const enum WasmEdge_Mutability Mut) {
  return toGlobTypeCxt(new WasmEdge::AST::GlobalType(
      genValType(ValType), static_cast<WasmEdge::ValMut>(Mut)));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValType
WasmEdge_GlobalTypeGetValType(const WasmEdge_GlobalTypeContext *Cxt) {
  if (Cxt) {
    return genWasmEdge_ValType(fromGlobTypeCxt(Cxt)->getValType());
  }
  return WasmEdge_ValTypeGenI32();
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_Mutability
WasmEdge_GlobalTypeGetMutability(const WasmEdge_GlobalTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_Mutability>(fromGlobTypeCxt(Cxt)->getValMut());
  }
  return WasmEdge_Mutability_Const;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_GlobalTypeDelete(WasmEdge_GlobalTypeContext *Cxt) {
  delete fromGlobTypeCxt(Cxt);
}

// <<<<<<<< WasmEdge global type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge import type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT enum WasmEdge_ExternalType
WasmEdge_ImportTypeGetExternalType(const WasmEdge_ImportTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_ExternalType>(
        fromImpTypeCxt(Cxt)->getExternalType());
  }
  return WasmEdge_ExternalType_Function;
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_ImportTypeGetModuleName(const WasmEdge_ImportTypeContext *Cxt) {
  if (Cxt) {
    auto StrView = fromImpTypeCxt(Cxt)->getModuleName();
    return WasmEdge_String{/* Length */ static_cast<uint32_t>(StrView.length()),
                           /* Buf */ StrView.data()};
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_ImportTypeGetExternalName(const WasmEdge_ImportTypeContext *Cxt) {
  if (Cxt) {
    auto StrView = fromImpTypeCxt(Cxt)->getExternalName();
    return WasmEdge_String{/* Length */ static_cast<uint32_t>(StrView.length()),
                           /* Buf */ StrView.data()};
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_ImportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt,
                                   const WasmEdge_ImportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromImpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Function) {
    uint32_t Idx = fromImpTypeCxt(Cxt)->getExternalFuncTypeIdx();
    auto SubTypes = fromASTModCxt(ASTCxt)->getTypeSection().getContent();
    if (Idx < SubTypes.size() && SubTypes[Idx].getCompositeType().isFunc()) {
      return toFuncTypeCxt(&(SubTypes[Idx].getCompositeType().getFuncType()));
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_TableTypeContext *
WasmEdge_ImportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt,
                                const WasmEdge_ImportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromImpTypeCxt(Cxt)->getExternalType() == WasmEdge::ExternalType::Table) {
    return toTabTypeCxt(&fromImpTypeCxt(Cxt)->getExternalTableType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_MemoryTypeContext *
WasmEdge_ImportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ImportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromImpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Memory) {
    return toMemTypeCxt(&fromImpTypeCxt(Cxt)->getExternalMemoryType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_TagTypeContext *
WasmEdge_ImportTypeGetTagType(const WasmEdge_ASTModuleContext *ASTCxt,
                              const WasmEdge_ImportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromImpTypeCxt(Cxt)->getExternalType() == WasmEdge::ExternalType::Tag) {
    return toTagTypeCxt(&fromImpTypeCxt(Cxt)->getExternalTagType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_GlobalTypeContext *
WasmEdge_ImportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ImportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromImpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Global) {
    return toGlobTypeCxt(&fromImpTypeCxt(Cxt)->getExternalGlobalType());
  }
  return nullptr;
}

// <<<<<<<< WasmEdge import type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge export type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT enum WasmEdge_ExternalType
WasmEdge_ExportTypeGetExternalType(const WasmEdge_ExportTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_ExternalType>(
        fromExpTypeCxt(Cxt)->getExternalType());
  }
  return WasmEdge_ExternalType_Function;
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_ExportTypeGetExternalName(const WasmEdge_ExportTypeContext *Cxt) {
  if (Cxt) {
    auto StrView = fromExpTypeCxt(Cxt)->getExternalName();
    return WasmEdge_String{/* Length */ static_cast<uint32_t>(StrView.length()),
                           /* Buf */ StrView.data()};
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_ExportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt,
                                   const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Function) {
    auto ImpDescs = fromASTModCxt(ASTCxt)->getImportSection().getContent();
    auto FuncIdxs = fromASTModCxt(ASTCxt)->getFunctionSection().getContent();
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();

    // Indexing the import descriptions.
    std::vector<uint32_t> ImpFuncs;
    ImpFuncs.reserve(ImpDescs.size());
    for (uint32_t I = 0; I < ImpDescs.size(); I++) {
      if (ImpDescs[I].getExternalType() == WasmEdge::ExternalType::Function) {
        ImpFuncs.push_back(I);
      }
    }
    // Get the function type index.
    uint32_t TypeIdx = 0;
    if (ExtIdx < ImpFuncs.size()) {
      // Imported function. Get the function type index from the import desc.
      TypeIdx = ImpDescs[ImpFuncs[ExtIdx]].getExternalFuncTypeIdx();
    } else if (ExtIdx < ImpFuncs.size() + FuncIdxs.size()) {
      // Module owned function. Get the function type index from the section.
      TypeIdx = FuncIdxs[ExtIdx - ImpFuncs.size()];
    } else {
      // Invalid function index.
      return nullptr;
    }
    // Get the function type.
    auto SubTypes = fromASTModCxt(ASTCxt)->getTypeSection().getContent();
    if (TypeIdx < SubTypes.size() &&
        SubTypes[TypeIdx].getCompositeType().isFunc()) {
      return toFuncTypeCxt(
          &(SubTypes[TypeIdx].getCompositeType().getFuncType()));
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_TableTypeContext *
WasmEdge_ExportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt,
                                const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() == WasmEdge::ExternalType::Table) {
    auto ImpDescs = fromASTModCxt(ASTCxt)->getImportSection().getContent();
    auto TabDescs = fromASTModCxt(ASTCxt)->getTableSection().getContent();
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();

    // Indexing the import descriptions.
    std::vector<uint32_t> ImpTabs;
    ImpTabs.reserve(ImpDescs.size());
    for (uint32_t I = 0; I < ImpDescs.size(); I++) {
      if (ImpDescs[I].getExternalType() == WasmEdge::ExternalType::Table) {
        ImpTabs.push_back(I);
      }
    }
    // Get the table type.
    if (ExtIdx < ImpTabs.size()) {
      // Imported table. Get the table type from the import desc.
      return toTabTypeCxt(&ImpDescs[ImpTabs[ExtIdx]].getExternalTableType());
    } else if (ExtIdx < ImpTabs.size() + TabDescs.size()) {
      // Module owned table. Get the table type from the section.
      return toTabTypeCxt(&TabDescs[ExtIdx - ImpTabs.size()].getTableType());
    } else {
      // Invalid table type index.
      return nullptr;
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_MemoryTypeContext *
WasmEdge_ExportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Memory) {
    auto ImpDescs = fromASTModCxt(ASTCxt)->getImportSection().getContent();
    auto MemTypes = fromASTModCxt(ASTCxt)->getMemorySection().getContent();
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();

    // Indexing the import descriptions.
    std::vector<uint32_t> ImpMems;
    ImpMems.reserve(ImpDescs.size());
    for (uint32_t I = 0; I < ImpDescs.size(); I++) {
      if (ImpDescs[I].getExternalType() == WasmEdge::ExternalType::Memory) {
        ImpMems.push_back(I);
      }
    }
    // Get the memory type.
    if (ExtIdx < ImpMems.size()) {
      // Imported memory. Get the memory type from the import desc.
      return toMemTypeCxt(&ImpDescs[ImpMems[ExtIdx]].getExternalMemoryType());
    } else if (ExtIdx < ImpMems.size() + MemTypes.size()) {
      // Module owned memory. Get the memory type from the section.
      return toMemTypeCxt(&MemTypes[ExtIdx - ImpMems.size()]);
    } else {
      // Invalid memory type index.
      return nullptr;
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_TagTypeContext *
WasmEdge_ExportTypeGetTagType(const WasmEdge_ASTModuleContext *ASTCxt,
                              const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() == WasmEdge::ExternalType::Tag) {
    // `external_index` = `tag_type_index` + `import_tag_nums`
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();
    const auto &ImpDescs =
        fromASTModCxt(ASTCxt)->getImportSection().getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Tag) {
        ExtIdx--;
      }
    }
    // Get the tag type
    const auto &TagDescs = fromASTModCxt(ASTCxt)->getTagSection().getContent();
    if (ExtIdx >= TagDescs.size()) {
      return nullptr;
    }
    return toTagTypeCxt(&TagDescs[ExtIdx]);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_GlobalTypeContext *
WasmEdge_ExportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Global) {
    auto ImpDescs = fromASTModCxt(ASTCxt)->getImportSection().getContent();
    auto GlobDescs = fromASTModCxt(ASTCxt)->getGlobalSection().getContent();
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();

    // Indexing the import descriptions.
    std::vector<uint32_t> ImpGlobs;
    ImpGlobs.reserve(ImpDescs.size());
    for (uint32_t I = 0; I < ImpDescs.size(); I++) {
      if (ImpDescs[I].getExternalType() == WasmEdge::ExternalType::Global) {
        ImpGlobs.push_back(I);
      }
    }
    // Get the global type.
    if (ExtIdx < ImpGlobs.size()) {
      // Imported global. Get the global type from the import desc.
      return toGlobTypeCxt(&ImpDescs[ImpGlobs[ExtIdx]].getExternalGlobalType());
    } else if (ExtIdx < ImpGlobs.size() + GlobDescs.size()) {
      // Module owned global. Get the global type from the section.
      return toGlobTypeCxt(
          &GlobDescs[ExtIdx - ImpGlobs.size()].getGlobalType());
    } else {
      // Invalid global type index.
      return nullptr;
    }
  }
  return nullptr;
}

// <<<<<<<< WasmEdge export type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge AOT compiler functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_CompilerContext *
WasmEdge_CompilerCreate(const WasmEdge_ConfigureContext *ConfCxt
                        [[maybe_unused]]) {
#ifdef WASMEDGE_USE_LLVM
  // Set force interpreter here to load instructions of function body forcibly.
  if (ConfCxt) {
    WasmEdge::Configure CopyConf(ConfCxt->Conf);
    CopyConf.getRuntimeConfigure().setForceInterpreter(true);
    return new WasmEdge_CompilerContext(CopyConf);
  } else {
    WasmEdge::Configure CopyConf;
    CopyConf.getRuntimeConfigure().setForceInterpreter(true);
    return new WasmEdge_CompilerContext(CopyConf);
  }
#else
  return nullptr;
#endif
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_CompilerCompile(
    WasmEdge_CompilerContext *Cxt [[maybe_unused]],
    const char *InPath [[maybe_unused]], const char *OutPath [[maybe_unused]]) {
#ifdef WASMEDGE_USE_LLVM
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        std::filesystem::path InputPath = std::filesystem::absolute(InPath);
        std::filesystem::path OutputPath = std::filesystem::absolute(OutPath);
        std::vector<WasmEdge::Byte> Data;
        std::unique_ptr<WasmEdge::AST::Module> Module;
        return Cxt->Load.loadFile(InputPath)
            .and_then([&](auto Result) noexcept {
              Data = std::move(Result);
              return Cxt->Load.parseModule(Data);
            })
            .and_then([&](auto Result) noexcept {
              Module = std::move(Result);
              return Cxt->Valid.validate(*Module.get());
            })
            .and_then(
                [&]() noexcept { return Cxt->Compiler.compile(*Module.get()); })
            .and_then([&](auto Result) noexcept {
              return Cxt->CodeGen.codegen(Data, std::move(Result), OutputPath);
            });
      },
      EmptyThen, Cxt);
#else
  return genWasmEdge_Result(ErrCode::Value::AOTDisabled);
#endif
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_CompilerCompileFromBuffer(
    WasmEdge_CompilerContext *Cxt, const uint8_t *InBuffer,
    const uint64_t InBufferLen, const char *OutPath) {
  return WasmEdge_CompilerCompileFromBytes(
      Cxt, WasmEdge_BytesWrap(InBuffer, static_cast<uint32_t>(InBufferLen)),
      OutPath);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_CompilerCompileFromBytes(
    WasmEdge_CompilerContext *Cxt [[maybe_unused]],
    const WasmEdge_Bytes Bytes [[maybe_unused]],
    const char *OutPath [[maybe_unused]]) {
#ifdef WASMEDGE_USE_LLVM
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        std::filesystem::path OutputPath = std::filesystem::absolute(OutPath);
        auto Data = genSpan(Bytes.Buf, Bytes.Length);
        std::unique_ptr<WasmEdge::AST::Module> Module;
        return Cxt->Load.parseModule(Data)
            .and_then([&](auto Result) noexcept {
              Module = std::move(Result);
              return Cxt->Valid.validate(*Module);
            })
            .and_then([&]() noexcept { return Cxt->Compiler.compile(*Module); })
            .and_then([&](auto Result) noexcept {
              return Cxt->CodeGen.codegen(Data, std::move(Result), OutputPath);
            });
      },
      EmptyThen, Cxt);
#else
  return genWasmEdge_Result(ErrCode::Value::AOTDisabled);
#endif
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_CompilerDelete(WasmEdge_CompilerContext *Cxt) {
  delete Cxt;
}

// <<<<<<<< WasmEdge AOT compiler functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge loader functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_LoaderContext *
WasmEdge_LoaderCreate(const WasmEdge_ConfigureContext *ConfCxt) {
  if (ConfCxt) {
    return toLoaderCxt(new WasmEdge::Loader::Loader(
        ConfCxt->Conf, &WasmEdge::Executor::Executor::Intrinsics));
  } else {
    return toLoaderCxt(new WasmEdge::Loader::Loader(
        WasmEdge::Configure(), &WasmEdge::Executor::Executor::Intrinsics));
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_LoaderParseFromFile(
    WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module,
    const char *Path) {
  return wrap(
      [&]() {
        return fromLoaderCxt(Cxt)->parseModule(std::filesystem::absolute(Path));
      },
      [&](auto &&Res) { *Module = toASTModCxt((*Res).release()); }, Cxt,
      Module);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_LoaderParseFromBuffer(
    WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module,
    const uint8_t *Buf, const uint32_t BufLen) {
  return WasmEdge_LoaderParseFromBytes(Cxt, Module,
                                       WasmEdge_BytesWrap(Buf, BufLen));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_LoaderParseFromBytes(
    WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module,
    const WasmEdge_Bytes Bytes) {
  return wrap(
      [&]() {
        return fromLoaderCxt(Cxt)->parseModule(
            genSpan(Bytes.Buf, Bytes.Length));
      },
      [&](auto &&Res) { *Module = toASTModCxt((*Res).release()); }, Cxt,
      Module);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_LoaderSerializeASTModule(
    WasmEdge_LoaderContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt,
    WasmEdge_Bytes *Buf) {
  return wrap(
      [&]() {
        return fromLoaderCxt(Cxt)->serializeModule(*fromASTModCxt(ASTCxt));
      },
      [&](auto &&Res) {
        uint32_t Size = static_cast<uint32_t>((*Res).size());
        uint8_t *Bytes = new uint8_t[Size];
        std::copy_n((*Res).begin(), Size, Bytes);
        Buf->Length = Size;
        Buf->Buf = Bytes;
      },
      Cxt, ASTCxt, Buf);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_LoaderDelete(WasmEdge_LoaderContext *Cxt) {
  delete fromLoaderCxt(Cxt);
}

// <<<<<<<< WasmEdge loader functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge validator functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ValidatorContext *
WasmEdge_ValidatorCreate(const WasmEdge_ConfigureContext *ConfCxt) {
  if (ConfCxt) {
    return toValidatorCxt(new WasmEdge::Validator::Validator(ConfCxt->Conf));
  } else {
    return toValidatorCxt(
        new WasmEdge::Validator::Validator(WasmEdge::Configure()));
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_ValidatorValidate(WasmEdge_ValidatorContext *Cxt,
                           const WasmEdge_ASTModuleContext *ModuleCxt) {
  return wrap(
      [&]() {
        return fromValidatorCxt(Cxt)->validate(*fromASTModCxt(ModuleCxt));
      },
      EmptyThen, Cxt, ModuleCxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ValidatorDelete(WasmEdge_ValidatorContext *Cxt) {
  delete fromValidatorCxt(Cxt);
}

// <<<<<<<< WasmEdge validator functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge executor functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ExecutorContext *
WasmEdge_ExecutorCreate(const WasmEdge_ConfigureContext *ConfCxt,
                        WasmEdge_StatisticsContext *StatCxt) {
  if (ConfCxt) {
    if (StatCxt) {
      return toExecutorCxt(new WasmEdge::Executor::Executor(
          ConfCxt->Conf, fromStatCxt(StatCxt)));
    } else {
      return toExecutorCxt(new WasmEdge::Executor::Executor(ConfCxt->Conf));
    }
  } else {
    if (StatCxt) {
      return toExecutorCxt(new WasmEdge::Executor::Executor(
          WasmEdge::Configure(), fromStatCxt(StatCxt)));
    } else {
      return toExecutorCxt(
          new WasmEdge::Executor::Executor(WasmEdge::Configure()));
    }
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorInstantiate(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_ModuleInstanceContext **ModuleCxt,
    WasmEdge_StoreContext *StoreCxt, const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return fromExecutorCxt(Cxt)->instantiateModule(*fromStoreCxt(StoreCxt),
                                                       *fromASTModCxt(ASTCxt));
      },
      [&](auto &&Res) { *ModuleCxt = toModCxt((*Res).release()); }, Cxt,
      ModuleCxt, StoreCxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorRegister(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_ModuleInstanceContext **ModuleCxt,
    WasmEdge_StoreContext *StoreCxt, const WasmEdge_ASTModuleContext *ASTCxt,
    const WasmEdge_String ModuleName) {
  return wrap(
      [&]() {
        return fromExecutorCxt(Cxt)->registerModule(*fromStoreCxt(StoreCxt),
                                                    *fromASTModCxt(ASTCxt),
                                                    genStrView(ModuleName));
      },
      [&](auto &&Res) { *ModuleCxt = toModCxt((*Res).release()); }, Cxt,
      ModuleCxt, StoreCxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorRegisterImport(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ModuleInstanceContext *ImportCxt) {
  return wrap(
      [&]() {
        return fromExecutorCxt(Cxt)->registerModule(*fromStoreCxt(StoreCxt),
                                                    *fromModCxt(ImportCxt));
      },
      EmptyThen, Cxt, StoreCxt, ImportCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_ExecutorInvoke(WasmEdge_ExecutorContext *Cxt,
                        const WasmEdge_FunctionInstanceContext *FuncCxt,
                        const WasmEdge_Value *Params, const uint32_t ParamLen,
                        WasmEdge_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]()
          -> WasmEdge::Expect<
              std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>> {
        return fromExecutorCxt(Cxt)->invoke(fromFuncCxt(FuncCxt),
                                            ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      FuncCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *
WasmEdge_ExecutorAsyncInvoke(WasmEdge_ExecutorContext *Cxt,
                             const WasmEdge_FunctionInstanceContext *FuncCxt,
                             const WasmEdge_Value *Params,
                             const uint32_t ParamLen) {
  if (Cxt && FuncCxt) {
    auto ParamPair = genParamPair(Params, ParamLen);
    return new WasmEdge_Async(fromExecutorCxt(Cxt)->asyncInvoke(
        fromFuncCxt(FuncCxt), ParamPair.first, ParamPair.second));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ExecutorDelete(WasmEdge_ExecutorContext *Cxt) {
  delete fromExecutorCxt(Cxt);
}

// <<<<<<<< WasmEdge executor functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_StoreContext *WasmEdge_StoreCreate(void) {
  return toStoreCxt(new WasmEdge::Runtime::StoreManager);
}

WASMEDGE_CAPI_EXPORT const WasmEdge_ModuleInstanceContext *
WasmEdge_StoreFindModule(const WasmEdge_StoreContext *Cxt,
                         const WasmEdge_String Name) {
  if (Cxt) {
    return toModCxt(fromStoreCxt(Cxt)->findModule(genStrView(Name)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListModuleLength(const WasmEdge_StoreContext *Cxt) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getModuleListSize();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListModule(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getModuleList(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_StoreDelete(WasmEdge_StoreContext *Cxt) {
  delete fromStoreCxt(Cxt);
}

// <<<<<<<< WasmEdge store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge module instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreate(const WasmEdge_String ModuleName) {
  return toModCxt(
      new WasmEdge::Runtime::Instance::ModuleInstance(genStrView(ModuleName)));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreateWASI(const char *const *Args,
                                  const uint32_t ArgLen,
                                  const char *const *Envs,
                                  const uint32_t EnvLen,
                                  const char *const *Preopens,
                                  const uint32_t PreopenLen) {
  auto *WasiMod = new WasmEdge::Host::WasiModule();
  WasmEdge_ModuleInstanceInitWASI(toModCxt(WasiMod), Args, ArgLen, Envs, EnvLen,
                                  Preopens, PreopenLen);
  return toModCxt(WasiMod);
}

WASMEDGE_CAPI_EXPORT extern WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreateWithData(const WasmEdge_String ModuleName,
                                      void *HostData,
                                      void (*Finalizer)(void *)) {
  return toModCxt(new WasmEdge::Runtime::Instance::ModuleInstance(
      genStrView(ModuleName), HostData, Finalizer));
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ModuleInstanceInitWASI(
    WasmEdge_ModuleInstanceContext *Cxt, const char *const *Args,
    const uint32_t ArgLen, const char *const *Envs, const uint32_t EnvLen,
    const char *const *Preopens, const uint32_t PreopenLen) {
  if (!Cxt) {
    return;
  }
  auto *WasiMod = dynamic_cast<WasmEdge::Host::WasiModule *>(fromModCxt(Cxt));
  if (!WasiMod) {
    return;
  }
  std::vector<std::string> ArgVec, EnvVec, DirVec;
  std::string ProgName;
  if (Args) {
    if (ArgLen > 0) {
      ProgName = Args[0];
    }
    for (uint32_t I = 1; I < ArgLen; I++) {
      ArgVec.emplace_back(Args[I]);
    }
  }
  if (Envs) {
    for (uint32_t I = 0; I < EnvLen; I++) {
      EnvVec.emplace_back(Envs[I]);
    }
  }
  if (Preopens) {
    for (uint32_t I = 0; I < PreopenLen; I++) {
      DirVec.emplace_back(Preopens[I]);
    }
  }
  auto &WasiEnv = WasiMod->getEnv();
  WasiEnv.init(DirVec, ProgName, ArgVec, EnvVec);
}

WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceWASIGetNativeHandler(
    const WasmEdge_ModuleInstanceContext *Cxt, int32_t Fd,
    uint64_t *NativeHandler) {
  if (!Cxt) {
    return 1;
  }
  auto *WasiMod =
      dynamic_cast<const WasmEdge::Host::WasiModule *>(fromModCxt(Cxt));
  if (!WasiMod) {
    return 2;
  }
  auto Handler = WasiMod->getEnv().getNativeHandler(Fd);
  if (!Handler) {
    return 2;
  }
  *NativeHandler = *Handler;
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ModuleInstanceWASIGetExitCode(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (!Cxt) {
    return EXIT_FAILURE;
  }
  auto *WasiMod =
      dynamic_cast<const WasmEdge::Host::WasiModule *>(fromModCxt(Cxt));
  if (!WasiMod) {
    return EXIT_FAILURE;
  }
  return WasiMod->getEnv().getExitCode();
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ModuleInstanceInitWasmEdgeProcess(const char *const *AllowedCmds,
                                           const uint32_t CmdsLen,
                                           const bool AllowAll) {
  using namespace std::literals::string_view_literals;
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_process"sv)) {
    PO::ArgumentParser Parser;
    Plugin->registerOptions(Parser);
    Parser.set_raw_value<std::vector<std::string>>(
        "allow-command"sv,
        std::vector<std::string>(AllowedCmds, AllowedCmds + CmdsLen));
    if (AllowAll) {
      Parser.set_raw_value("allow-command-all"sv);
    }
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_String WasmEdge_ModuleInstanceGetModuleName(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    auto StrView = fromModCxt(Cxt)->getModuleName();
    return WasmEdge_String{/* Length */ static_cast<uint32_t>(StrView.length()),
                           /* Buf */ StrView.data()};
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT void *
WasmEdge_ModuleInstanceGetHostData(const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    return fromModCxt(Cxt)->getHostData();
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionInstanceContext *
WasmEdge_ModuleInstanceFindFunction(const WasmEdge_ModuleInstanceContext *Cxt,
                                    const WasmEdge_String Name) {
  if (Cxt) {
    return toFuncCxt(fromModCxt(Cxt)->findFuncExports(genStrView(Name)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_TableInstanceContext *
WasmEdge_ModuleInstanceFindTable(const WasmEdge_ModuleInstanceContext *Cxt,
                                 const WasmEdge_String Name) {
  if (Cxt) {
    return toTabCxt(fromModCxt(Cxt)->findTableExports(genStrView(Name)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryInstanceContext *
WasmEdge_ModuleInstanceFindMemory(const WasmEdge_ModuleInstanceContext *Cxt,
                                  const WasmEdge_String Name) {
  if (Cxt) {
    return toMemCxt(fromModCxt(Cxt)->findMemoryExports(genStrView(Name)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_TagInstanceContext *
WasmEdge_ModuleInstanceFindTag(const WasmEdge_ModuleInstanceContext *Cxt,
                               const WasmEdge_String Name) {
  if (Cxt) {
    return toTagCxt(fromModCxt(Cxt)->findTagExports(genStrView(Name)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalInstanceContext *
WasmEdge_ModuleInstanceFindGlobal(const WasmEdge_ModuleInstanceContext *Cxt,
                                  const WasmEdge_String Name) {
  if (Cxt) {
    return toGlobCxt(fromModCxt(Cxt)->findGlobalExports(genStrView(Name)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ModuleInstanceListFunctionLength(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    return fromModCxt(Cxt)->getFuncExportNum();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListFunction(const WasmEdge_ModuleInstanceContext *Cxt,
                                    WasmEdge_String *Names,
                                    const uint32_t Len) {
  if (Cxt) {
    return fromModCxt(Cxt)->getFuncExports(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ModuleInstanceListTableLength(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    return fromModCxt(Cxt)->getTableExportNum();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ModuleInstanceListTable(const WasmEdge_ModuleInstanceContext *Cxt,
                                 WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fromModCxt(Cxt)->getTableExports(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ModuleInstanceListMemoryLength(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    return fromModCxt(Cxt)->getMemoryExportNum();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ModuleInstanceListMemory(const WasmEdge_ModuleInstanceContext *Cxt,
                                  WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fromModCxt(Cxt)->getMemoryExports(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ModuleInstanceListTagLength(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    return fromModCxt(Cxt)->getTagExportNum();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ModuleInstanceListTag(const WasmEdge_ModuleInstanceContext *Cxt,
                               WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fromModCxt(Cxt)->getTagExports(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ModuleInstanceListGlobalLength(
    const WasmEdge_ModuleInstanceContext *Cxt) {
  if (Cxt) {
    return fromModCxt(Cxt)->getGlobalExportNum();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ModuleInstanceListGlobal(const WasmEdge_ModuleInstanceContext *Cxt,
                                  WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fromModCxt(Cxt)->getGlobalExports(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ModuleInstanceAddFunction(WasmEdge_ModuleInstanceContext *Cxt,
                                   const WasmEdge_String Name,
                                   WasmEdge_FunctionInstanceContext *FuncCxt) {
  if (Cxt && FuncCxt) {
    fromModCxt(Cxt)->addHostFunc(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::FunctionInstance>(
            fromFuncCxt(FuncCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ModuleInstanceAddTable(WasmEdge_ModuleInstanceContext *Cxt,
                                const WasmEdge_String Name,
                                WasmEdge_TableInstanceContext *TableCxt) {
  if (Cxt && TableCxt) {
    fromModCxt(Cxt)->addHostTable(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::TableInstance>(
            fromTabCxt(TableCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ModuleInstanceAddMemory(WasmEdge_ModuleInstanceContext *Cxt,
                                 const WasmEdge_String Name,
                                 WasmEdge_MemoryInstanceContext *MemoryCxt) {
  if (Cxt && MemoryCxt) {
    fromModCxt(Cxt)->addHostMemory(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::MemoryInstance>(
            fromMemCxt(MemoryCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ModuleInstanceAddGlobal(WasmEdge_ModuleInstanceContext *Cxt,
                                 const WasmEdge_String Name,
                                 WasmEdge_GlobalInstanceContext *GlobalCxt) {
  if (Cxt && GlobalCxt) {
    fromModCxt(Cxt)->addHostGlobal(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::GlobalInstance>(
            fromGlobCxt(GlobalCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ModuleInstanceDelete(WasmEdge_ModuleInstanceContext *Cxt) {
  delete fromModCxt(Cxt);
}

// <<<<<<<< WasmEdge module instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge function instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionInstanceContext *
WasmEdge_FunctionInstanceCreate(const WasmEdge_FunctionTypeContext *Type,
                                WasmEdge_HostFunc_t HostFunc, void *Data,
                                const uint64_t Cost) {
  if (Type && HostFunc) {
    return toFuncCxt(new WasmEdge::Runtime::Instance::FunctionInstance(
        std::make_unique<CAPIHostFunc>(fromFuncTypeCxt(Type), HostFunc, Data,
                                       Cost)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionInstanceContext *
WasmEdge_FunctionInstanceCreateBinding(const WasmEdge_FunctionTypeContext *Type,
                                       WasmEdge_WrapFunc_t WrapFunc,
                                       void *Binding, void *Data,
                                       const uint64_t Cost) {
  if (Type && WrapFunc) {
    return toFuncCxt(new WasmEdge::Runtime::Instance::FunctionInstance(
        std::make_unique<CAPIHostFunc>(fromFuncTypeCxt(Type), WrapFunc, Binding,
                                       Data, Cost)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_FunctionInstanceGetFunctionType(
    const WasmEdge_FunctionInstanceContext *Cxt) {
  if (Cxt) {
    return toFuncTypeCxt(&fromFuncCxt(Cxt)->getFuncType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT extern const void *
WasmEdge_FunctionInstanceGetData(const WasmEdge_FunctionInstanceContext *Cxt) {
  if (Cxt) {
    return reinterpret_cast<CAPIHostFunc *>(&fromFuncCxt(Cxt)->getHostFunc())
        ->getData();
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_FunctionInstanceDelete(WasmEdge_FunctionInstanceContext *Cxt) {
  delete fromFuncCxt(Cxt);
}

// <<<<<<<< WasmEdge function instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge table instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_TableInstanceContext *
WasmEdge_TableInstanceCreate(const WasmEdge_TableTypeContext *TabType) {
  if (TabType) {
    const AST::TableType &TType = *fromTabTypeCxt(TabType);
    if (!TType.getRefType().isNullableRefType()) {
      spdlog::error(WasmEdge::ErrCode::Value::NonNullRequired);
      return nullptr;
    }
    return toTabCxt(new WasmEdge::Runtime::Instance::TableInstance(TType));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT extern WasmEdge_TableInstanceContext *
WasmEdge_TableInstanceCreateWithInit(const WasmEdge_TableTypeContext *TabType,
                                     const WasmEdge_Value Value) {
  if (TabType) {
    // Comparison of the value types needs the module instance to retrieve the
    // function type index after applying the typed function reference proposal.
    // It's impossible to do this without refactoring. Therefore simply match
    // the FuncRef and ExternRef here.
    const AST::TableType &TType = *fromTabTypeCxt(TabType);
    WasmEdge::ValType GotType = genValType(Value.Type);
    if (TType.getRefType().isFuncRefType() != GotType.isFuncRefType()) {
      spdlog::error(WasmEdge::ErrCode::Value::RefTypeMismatch);
      spdlog::error(
          WasmEdge::ErrInfo::InfoMismatch(TType.getRefType(), GotType));
      return nullptr;
    }
    auto Val = WasmEdge::ValVariant(
                   to_WasmEdge_128_t<WasmEdge::uint128_t>(Value.Value))
                   .get<WasmEdge::RefVariant>();
    if (!TType.getRefType().isNullableRefType() && Val.isNull()) {
      spdlog::error(WasmEdge::ErrCode::Value::NonNullRequired);
      return nullptr;
    }
    return toTabCxt(new WasmEdge::Runtime::Instance::TableInstance(TType, Val));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_TableTypeContext *
WasmEdge_TableInstanceGetTableType(const WasmEdge_TableInstanceContext *Cxt) {
  if (Cxt) {
    return toTabTypeCxt(&fromTabCxt(Cxt)->getTableType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_TableInstanceGetData(const WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value *Data, const uint32_t Offset) {
  return wrap([&]() { return fromTabCxt(Cxt)->getRefAddr(Offset); },
              [&Data, &Cxt](auto &&Res) {
                *Data = genWasmEdge_Value(
                    *Res, fromTabCxt(Cxt)->getTableType().getRefType());
              },
              Cxt, Data);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_TableInstanceSetData(WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value Data, const uint32_t Offset) {
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        // Comparison of the value types needs the module instance to retrieve
        // the function type index after applying the typed function reference
        // proposal. It's impossible to do this without refactoring. Therefore
        // simply match the FuncRef and ExternRef here.
        WasmEdge::ValType ExpType =
            fromTabCxt(Cxt)->getTableType().getRefType();
        WasmEdge::ValType GotType = genValType(Data.Type);
        if (!GotType.isRefType() ||
            ExpType.isFuncRefType() != GotType.isFuncRefType()) {
          spdlog::error(WasmEdge::ErrCode::Value::RefTypeMismatch);
          spdlog::error(WasmEdge::ErrInfo::InfoMismatch(ExpType, GotType));
          return Unexpect(WasmEdge::ErrCode::Value::RefTypeMismatch);
        }
        auto Val = WasmEdge::ValVariant(
                       to_WasmEdge_128_t<WasmEdge::uint128_t>(Data.Value))
                       .get<WasmEdge::RefVariant>();
        if (!ExpType.isNullableRefType() && Val.isNull()) {
          // If this table is not a nullable ref type, the data should not be
          // null.
          spdlog::error(WasmEdge::ErrCode::Value::NonNullRequired);
          return Unexpect(WasmEdge::ErrCode::Value::NonNullRequired);
        }
        return fromTabCxt(Cxt)->setRefAddr(Offset, Val);
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_TableInstanceGetSize(const WasmEdge_TableInstanceContext *Cxt) {
  if (Cxt) {
    return fromTabCxt(Cxt)->getSize();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_TableInstanceGrow(
    WasmEdge_TableInstanceContext *Cxt, const uint32_t Size) {
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        if (fromTabCxt(Cxt)->growTable(Size)) {
          return {};
        } else {
          spdlog::error(WasmEdge::ErrCode::Value::TableOutOfBounds);
          return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::TableOutOfBounds);
        }
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_TableInstanceDelete(WasmEdge_TableInstanceContext *Cxt) {
  delete fromTabCxt(Cxt);
}

// <<<<<<<< WasmEdge table instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge memory instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryInstanceContext *
WasmEdge_MemoryInstanceCreate(const WasmEdge_MemoryTypeContext *MemType) {
  if (MemType) {
    return toMemCxt(new WasmEdge::Runtime::Instance::MemoryInstance(
        *fromMemTypeCxt(MemType)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_MemoryTypeContext *
WasmEdge_MemoryInstanceGetMemoryType(
    const WasmEdge_MemoryInstanceContext *Cxt) {
  if (Cxt) {
    return toMemTypeCxt(&fromMemCxt(Cxt)->getMemoryType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_MemoryInstanceGetData(
    const WasmEdge_MemoryInstanceContext *Cxt, uint8_t *Data,
    const uint32_t Offset, const uint32_t Length) {
  return wrap([&]() { return fromMemCxt(Cxt)->getBytes(Offset, Length); },
              [&](auto &&Res) { std::copy_n((*Res).begin(), Length, Data); },
              Cxt, Data);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_MemoryInstanceSetData(
    WasmEdge_MemoryInstanceContext *Cxt, const uint8_t *Data,
    const uint32_t Offset, const uint32_t Length) {
  return wrap(
      [&]() {
        return fromMemCxt(Cxt)->setBytes(genSpan(Data, Length), Offset, 0,
                                         Length);
      },
      EmptyThen, Cxt, Data);
}

WASMEDGE_CAPI_EXPORT uint8_t *
WasmEdge_MemoryInstanceGetPointer(WasmEdge_MemoryInstanceContext *Cxt,
                                  const uint32_t Offset,
                                  const uint32_t Length) {
  if (Cxt) {
    const auto S = fromMemCxt(Cxt)->getSpan<uint8_t>(Offset, Length);
    if (S.size() == Length) {
      return S.data();
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const uint8_t *WasmEdge_MemoryInstanceGetPointerConst(
    const WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Offset,
    const uint32_t Length) {
  if (Cxt) {
    const auto S = fromMemCxt(Cxt)->getSpan<const uint8_t>(Offset, Length);
    if (S.size() == Length) {
      return S.data();
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_MemoryInstanceGetPageSize(const WasmEdge_MemoryInstanceContext *Cxt) {
  if (Cxt) {
    return fromMemCxt(Cxt)->getPageSize();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_MemoryInstanceGrowPage(
    WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Page) {
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        if (fromMemCxt(Cxt)->growPage(Page)) {
          return {};
        } else {
          spdlog::error(WasmEdge::ErrCode::Value::MemoryOutOfBounds);
          return WasmEdge::Unexpect(
              WasmEdge::ErrCode::Value::MemoryOutOfBounds);
        }
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_MemoryInstanceDelete(WasmEdge_MemoryInstanceContext *Cxt) {
  delete fromMemCxt(Cxt);
}

// <<<<<<<< WasmEdge memory instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge tag instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT const WasmEdge_TagTypeContext *
WasmEdge_TagInstanceGetTagType(const WasmEdge_TagInstanceContext *Cxt) {
  if (Cxt) {
    return toTagTypeCxt(&fromTagCxt(Cxt)->getTagType());
  }
  return nullptr;
}

// <<<<<<<< WasmEdge tag instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge global instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalInstanceContext *
WasmEdge_GlobalInstanceCreate(const WasmEdge_GlobalTypeContext *GlobType,
                              const WasmEdge_Value Value) {
  if (GlobType) {
    // Comparison of the value types needs the module instance to retrieve the
    // function type index after applying the typed function reference proposal.
    // It's impossible to do this without refactoring. Therefore simply match
    // the FuncRef and ExternRef here.
    const AST::GlobalType &GType = *fromGlobTypeCxt(GlobType);
    WasmEdge::ValType ExpType = GType.getValType();
    WasmEdge::ValType GotType = genValType(Value.Type);
    if (ExpType.isFuncRefType() != GotType.isFuncRefType()) {
      spdlog::error(WasmEdge::ErrCode::Value::SetValueErrorType);
      spdlog::error(WasmEdge::ErrInfo::InfoMismatch(ExpType, GotType));
      return nullptr;
    }

    WasmEdge::ValVariant Val =
        to_WasmEdge_128_t<WasmEdge::uint128_t>(Value.Value);
    if (ExpType.isRefType()) {
      // Reference type case.
      if (!ExpType.isNullableRefType() &&
          Val.get<WasmEdge::RefVariant>().isNull()) {
        // If this global is not a nullable ref type, the data should not be
        // null.
        spdlog::error(WasmEdge::ErrCode::Value::NonNullRequired);
        return nullptr;
      }
    } else {
      // Number type case.
      if (ExpType != GotType) {
        spdlog::error(WasmEdge::ErrCode::Value::SetValueErrorType);
        return nullptr;
      }
    }
    return toGlobCxt(
        new WasmEdge::Runtime::Instance::GlobalInstance(GType, Val));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_GlobalTypeContext *
WasmEdge_GlobalInstanceGetGlobalType(
    const WasmEdge_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return toGlobTypeCxt(&fromGlobCxt(Cxt)->getGlobalType());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value
WasmEdge_GlobalInstanceGetValue(const WasmEdge_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return genWasmEdge_Value(fromGlobCxt(Cxt)->getValue(),
                             fromGlobCxt(Cxt)->getGlobalType().getValType());
  }
  return genWasmEdge_Value(
      WasmEdge::ValVariant(static_cast<WasmEdge::uint128_t>(0)), TypeCode::I32);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_GlobalInstanceSetValue(
    WasmEdge_GlobalInstanceContext *Cxt, const WasmEdge_Value Value) {
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        const auto &GlobType = fromGlobCxt(Cxt)->getGlobalType();
        if (GlobType.getValMut() != WasmEdge::ValMut::Var) {
          spdlog::error(WasmEdge::ErrCode::Value::SetValueToConst);
          return Unexpect(WasmEdge::ErrCode::Value::SetValueToConst);
        }

        // Comparison of the value types needs the module instance to retrieve
        // the function type index after applying the typed function reference
        // proposal. It's impossible to do this without refactoring. Therefore
        // simply match the FuncRef and ExternRef here.
        WasmEdge::ValType ExpType = GlobType.getValType();
        WasmEdge::ValType GotType = genValType(Value.Type);
        if (ExpType.isRefType() &&
            ExpType.isFuncRefType() != GotType.isFuncRefType()) {
          spdlog::error(WasmEdge::ErrCode::Value::RefTypeMismatch);
          spdlog::error(WasmEdge::ErrInfo::InfoMismatch(ExpType, GotType));
          return Unexpect(WasmEdge::ErrCode::Value::RefTypeMismatch);
        }

        WasmEdge::ValVariant Val =
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Value.Value);
        if (ExpType.isRefType()) {
          // Reference type case.
          if (!ExpType.isNullableRefType() &&
              Val.get<WasmEdge::RefVariant>().isNull()) {
            // If this global is not a nullable ref type, the data should not be
            // null.
            spdlog::error(WasmEdge::ErrCode::Value::NonNullRequired);
            return Unexpect(WasmEdge::ErrCode::Value::NonNullRequired);
          }
        } else {
          // Number type case.
          if (ExpType != GotType) {
            spdlog::error(WasmEdge::ErrCode::Value::SetValueErrorType);
            return Unexpect(WasmEdge::ErrCode::Value::SetValueErrorType);
          }
        }
        fromGlobCxt(Cxt)->setValue(Val);
        return {};
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_GlobalInstanceDelete(WasmEdge_GlobalInstanceContext *Cxt) {
  delete fromGlobCxt(Cxt);
}

// <<<<<<<< WasmEdge global instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge calling frame functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ExecutorContext *
WasmEdge_CallingFrameGetExecutor(const WasmEdge_CallingFrameContext *Cxt) {
  if (Cxt) {
    return toExecutorCxt(fromCallFrameCxt(Cxt)->getExecutor());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_ModuleInstanceContext *
WasmEdge_CallingFrameGetModuleInstance(
    const WasmEdge_CallingFrameContext *Cxt) {
  if (Cxt) {
    return toModCxt(fromCallFrameCxt(Cxt)->getModule());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryInstanceContext *
WasmEdge_CallingFrameGetMemoryInstance(const WasmEdge_CallingFrameContext *Cxt,
                                       const uint32_t Idx) {
  if (Cxt) {
    return toMemCxt(fromCallFrameCxt(Cxt)->getMemoryByIndex(Idx));
  }
  return nullptr;
}

// <<<<<<<< WasmEdge calling frame functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Async functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncWait(const WasmEdge_Async *Cxt) {
  if (Cxt) {
    Cxt->Async.wait();
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_AsyncWaitFor(const WasmEdge_Async *Cxt,
                                                uint64_t Milliseconds) {
  if (Cxt) {
    return Cxt->Async.waitFor(std::chrono::milliseconds(Milliseconds));
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncCancel(WasmEdge_Async *Cxt) {
  if (Cxt) {
    Cxt->Async.cancel();
  }
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_AsyncGetReturnsLength(const WasmEdge_Async *Cxt) {
  if (Cxt) {
    if (auto Res = Cxt->Async.get()) {
      return static_cast<uint32_t>((*Res).size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_AsyncGet(const WasmEdge_Async *Cxt, WasmEdge_Value *Returns,
                  const uint32_t ReturnLen) {
  return wrap(
      [&]() { return Cxt->Async.get(); },
      [&](auto Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncDelete(WasmEdge_Async *Cxt) {
  delete Cxt;
}

// <<<<<<<< WasmEdge Async functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge VM functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_VMContext *
WasmEdge_VMCreate(const WasmEdge_ConfigureContext *ConfCxt,
                  WasmEdge_StoreContext *StoreCxt) {
  if (ConfCxt) {
    if (StoreCxt) {
      return new WasmEdge_VMContext(ConfCxt->Conf, *fromStoreCxt(StoreCxt));
    } else {
      return new WasmEdge_VMContext(ConfCxt->Conf);
    }
  } else {
    if (StoreCxt) {
      return new WasmEdge_VMContext(WasmEdge::Configure(),
                                    *fromStoreCxt(StoreCxt));
    } else {
      return new WasmEdge_VMContext(WasmEdge::Configure());
    }
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromFile(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const char *Path) {
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      std::filesystem::absolute(Path));
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromBuffer(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const uint8_t *Buf, const uint32_t BufLen) {
  return WasmEdge_VMRegisterModuleFromBytes(Cxt, ModuleName,
                                            WasmEdge_BytesWrap(Buf, BufLen));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromBytes(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const WasmEdge_Bytes Bytes) {
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      genSpan(Bytes.Buf, Bytes.Length));
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      *fromASTModCxt(ASTCxt));
      },
      EmptyThen, Cxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromImport(
    WasmEdge_VMContext *Cxt, const WasmEdge_ModuleInstanceContext *ImportCxt) {
  return wrap([&]() { return Cxt->VM.registerModule(*fromModCxt(ImportCxt)); },
              EmptyThen, Cxt, ImportCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRunWasmFromFile(
    WasmEdge_VMContext *Cxt, const char *Path, const WasmEdge_String FuncName,
    const WasmEdge_Value *Params, const uint32_t ParamLen,
    WasmEdge_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(std::filesystem::absolute(Path),
                                   genStrView(FuncName), ParamPair.first,
                                   ParamPair.second);
      },
      [&](auto Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRunWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns,
    const uint32_t ReturnLen) {
  return WasmEdge_VMRunWasmFromBytes(Cxt, WasmEdge_BytesWrap(Buf, BufLen),
                                     FuncName, Params, ParamLen, Returns,
                                     ReturnLen);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRunWasmFromBytes(
    WasmEdge_VMContext *Cxt, const WasmEdge_Bytes Bytes,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns,
    const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(genSpan(Bytes.Buf, Bytes.Length),
                                   genStrView(FuncName), ParamPair.first,
                                   ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); },
      Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRunWasmFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns,
    const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(*fromASTModCxt(ASTCxt), genStrView(FuncName),
                                   ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromFile(
    WasmEdge_VMContext *Cxt, const char *Path, const WasmEdge_String FuncName,
    const WasmEdge_Value *Params, const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  if (Cxt) {
    return new WasmEdge_Async(Cxt->VM.asyncRunWasmFile(
        std::filesystem::absolute(Path), genStrView(FuncName), ParamPair.first,
        ParamPair.second));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  return WasmEdge_VMAsyncRunWasmFromBytes(Cxt, WasmEdge_BytesWrap(Buf, BufLen),
                                          FuncName, Params, ParamLen);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromBytes(
    WasmEdge_VMContext *Cxt, const WasmEdge_Bytes Bytes,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  if (Cxt) {
    return new WasmEdge_Async(Cxt->VM.asyncRunWasmFile(
        genSpan(Bytes.Buf, Bytes.Length), genStrView(FuncName), ParamPair.first,
        ParamPair.second));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  if (Cxt && ASTCxt) {
    return new WasmEdge_Async(
        Cxt->VM.asyncRunWasmFile(*fromASTModCxt(ASTCxt), genStrView(FuncName),
                                 ParamPair.first, ParamPair.second));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_VMLoadWasmFromFile(WasmEdge_VMContext *Cxt, const char *Path) {
  return wrap(
      [&]() { return Cxt->VM.loadWasm(std::filesystem::absolute(Path)); },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMLoadWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen) {
  return WasmEdge_VMLoadWasmFromBytes(Cxt, WasmEdge_BytesWrap(Buf, BufLen));
}

WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_VMLoadWasmFromBytes(WasmEdge_VMContext *Cxt,
                             const WasmEdge_Bytes Bytes) {
  return wrap(
      [&]() { return Cxt->VM.loadWasm(genSpan(Bytes.Buf, Bytes.Length)); },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMLoadWasmFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap([&]() { return Cxt->VM.loadWasm(*fromASTModCxt(ASTCxt)); },
              EmptyThen, Cxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_VMValidate(WasmEdge_VMContext *Cxt) {
  return wrap([&]() { return Cxt->VM.validate(); }, EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_VMInstantiate(WasmEdge_VMContext *Cxt) {
  return wrap([&]() { return Cxt->VM.instantiate(); }, EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_VMExecute(WasmEdge_VMContext *Cxt, const WasmEdge_String FuncName,
                   const WasmEdge_Value *Params, const uint32_t ParamLen,
                   WasmEdge_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.execute(genStrView(FuncName), ParamPair.first,
                               ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); },
      Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMExecuteRegistered(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns,
    const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.execute(genStrView(ModuleName), genStrView(FuncName),
                               ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); },
      Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *
WasmEdge_VMAsyncExecute(WasmEdge_VMContext *Cxt, const WasmEdge_String FuncName,
                        const WasmEdge_Value *Params, const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  if (Cxt) {
    return new WasmEdge_Async(Cxt->VM.asyncExecute(
        genStrView(FuncName), ParamPair.first, ParamPair.second));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncExecuteRegistered(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  if (Cxt) {
    return new WasmEdge_Async(
        Cxt->VM.asyncExecute(genStrView(ModuleName), genStrView(FuncName),
                             ParamPair.first, ParamPair.second));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_VMGetFunctionType(const WasmEdge_VMContext *Cxt,
                           const WasmEdge_String FuncName) {
  if (Cxt) {
    const auto FuncList = Cxt->VM.getFunctionList();
    for (const auto &It : FuncList) {
      if (It.first == genStrView(FuncName)) {
        return toFuncTypeCxt(&It.second);
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_VMGetFunctionTypeRegistered(const WasmEdge_VMContext *Cxt,
                                     const WasmEdge_String ModuleName,
                                     const WasmEdge_String FuncName) {
  if (Cxt) {
    const auto *ModInst =
        Cxt->VM.getStoreManager().findModule(genStrView(ModuleName));
    if (ModInst != nullptr) {
      const auto *FuncInst = ModInst->findFuncExports(genStrView(FuncName));
      if (FuncInst != nullptr) {
        return toFuncTypeCxt(&FuncInst->getFuncType());
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_VMCleanup(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    Cxt->VM.cleanup();
  }
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_VMGetFunctionListLength(const WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(Cxt->VM.getFunctionList().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_VMGetFunctionList(
    const WasmEdge_VMContext *Cxt, WasmEdge_String *Names,
    const WasmEdge_FunctionTypeContext **FuncTypes, const uint32_t Len) {
  if (Cxt) {
    // Not to use VM::getFunctionList() here because not to allocate the
    // returned function name strings.
    const auto *ModInst = Cxt->VM.getActiveModule();
    if (ModInst != nullptr) {
      return ModInst->getFuncExports([&](const auto &FuncExp) {
        uint32_t I = 0;
        for (auto It = FuncExp.cbegin(); It != FuncExp.cend() && I < Len;
             It++, I++) {
          const auto *FuncInst = It->second;
          const auto &FuncType = FuncInst->getFuncType();
          if (Names) {
            Names[I] = WasmEdge_String{
                /* Length */ static_cast<uint32_t>(It->first.length()),
                /* Buf */ It->first.data()};
          }
          if (FuncTypes) {
            FuncTypes[I] = toFuncTypeCxt(&FuncType);
          }
        }
        return static_cast<uint32_t>(FuncExp.size());
      });
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT WasmEdge_ModuleInstanceContext *
WasmEdge_VMGetImportModuleContext(const WasmEdge_VMContext *Cxt,
                                  const enum WasmEdge_HostRegistration Reg) {
  if (Cxt) {
    return toModCxt(
        Cxt->VM.getImportModule(static_cast<WasmEdge::HostRegistration>(Reg)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_VMListRegisteredModuleLength(const WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return Cxt->VM.getStoreManager().getModuleListSize();
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_VMListRegisteredModule(
    const WasmEdge_VMContext *Cxt, WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return Cxt->VM.getStoreManager().getModuleList(
        [&](auto &Map) { return fillMap(Map, Names, Len); });
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_ModuleInstanceContext *
WasmEdge_VMGetRegisteredModule(const WasmEdge_VMContext *Cxt,
                               const WasmEdge_String ModuleName) {
  if (Cxt) {
    return toModCxt(
        Cxt->VM.getStoreManager().findModule(genStrView(ModuleName)));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_ModuleInstanceContext *
WasmEdge_VMGetActiveModule(const WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return toModCxt(Cxt->VM.getActiveModule());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_StoreContext *
WasmEdge_VMGetStoreContext(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return toStoreCxt(&Cxt->VM.getStoreManager());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_LoaderContext *
WasmEdge_VMGetLoaderContext(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return toLoaderCxt(&Cxt->VM.getLoader());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_ValidatorContext *
WasmEdge_VMGetValidatorContext(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return toValidatorCxt(&Cxt->VM.getValidator());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_ExecutorContext *
WasmEdge_VMGetExecutorContext(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return toExecutorCxt(&Cxt->VM.getExecutor());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_StatisticsContext *
WasmEdge_VMGetStatisticsContext(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return toStatCxt(&Cxt->VM.getStatistics());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_VMDelete(WasmEdge_VMContext *Cxt) {
  delete Cxt;
}

// <<<<<<<< WasmEdge VM functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Driver functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#if WASMEDGE_OS_WINDOWS
WASMEDGE_CAPI_EXPORT const char **
WasmEdge_Driver_ArgvCreate(int Argc, const wchar_t *Argv[]) {
  const Span<const wchar_t *> Args(Argv, static_cast<size_t>(Argc));
  const size_t PointerArraySize = static_cast<size_t>(Argc) * sizeof(char *);
  size_t StringBufferSize = 0;
  for (auto Arg : Args) {
    const auto Res = std::max<size_t>(
        static_cast<size_t>(winapi::WideCharToMultiByte(
            winapi::CP_UTF8_, 0, Arg, -1, nullptr, 0, nullptr, nullptr)),
        1u);
    StringBufferSize += Res;
  }
  auto Buffer = std::make_unique<char[]>(PointerArraySize + StringBufferSize);
  Span<char *> PointerArray(reinterpret_cast<char **>(Buffer.get()),
                            static_cast<size_t>(Argc));
  Span<char> StringBuffer(Buffer.get() + PointerArraySize, StringBufferSize);
  for (auto Arg : Args) {
    PointerArray[0] = StringBuffer.data();
    PointerArray = PointerArray.subspan(1);
    const auto Res = std::max<size_t>(
        static_cast<size_t>(winapi::WideCharToMultiByte(
            winapi::CP_UTF8_, 0, Arg, -1, StringBuffer.data(),
            static_cast<int>(StringBuffer.size()), nullptr, nullptr)),
        1);
    StringBuffer = StringBuffer.subspan(Res);
  }

  return reinterpret_cast<const char **>(Buffer.release());
}

WASMEDGE_CAPI_EXPORT void WasmEdge_Driver_ArgvDelete(const char *Argv[]) {
  std::unique_ptr<char[]> Buffer(reinterpret_cast<char *>(Argv));
  Buffer.reset();
}

WASMEDGE_CAPI_EXPORT void WasmEdge_Driver_SetConsoleOutputCPtoUTF8(void) {
#if WINAPI_PARTITION_DESKTOP
  winapi::SetConsoleOutputCP(winapi::CP_UTF8_);
#endif
}
#endif

WASMEDGE_CAPI_EXPORT int WasmEdge_Driver_Compiler(int Argc,
                                                  const char *Argv[]) {
  return WasmEdge::Driver::UniTool(Argc, Argv,
                                   WasmEdge::Driver::ToolType::Compiler);
}

WASMEDGE_CAPI_EXPORT int WasmEdge_Driver_Tool(int Argc, const char *Argv[]) {
  return WasmEdge::Driver::UniTool(Argc, Argv,
                                   WasmEdge::Driver::ToolType::Tool);
}

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
WASMEDGE_CAPI_EXPORT int WasmEdge_Driver_WasiNNRPCServer(int Argc,
                                                         const char *Argv[]) {
  // UniTool does not support ToolType::WasiNNRPCServer yet (to avoid #ifdef
  // hell)
  return WasmEdge::Driver::WasiNNRPCServer(Argc, Argv);
}
#endif

WASMEDGE_CAPI_EXPORT int WasmEdge_Driver_UniTool(int Argc, const char *Argv[]) {
  return WasmEdge::Driver::UniTool(Argc, Argv, WasmEdge::Driver::ToolType::All);
}

#ifdef WASMEDGE_BUILD_FUZZING
WASMEDGE_CAPI_EXPORT extern "C" int
WasmEdge_Driver_FuzzTool(const uint8_t *Data, size_t Size) {
  return WasmEdge::Driver::FuzzTool(Data, Size);
}

WASMEDGE_CAPI_EXPORT extern "C" int WasmEdge_Driver_FuzzPO(const uint8_t *Data,
                                                           size_t Size) {
  return WasmEdge::Driver::FuzzPO(Data, Size);
}
#endif

// <<<<<<<< WasmEdge Driver functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Plugin functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT void WasmEdge_PluginLoadWithDefaultPaths(void) {
  WasmEdge::Plugin::Plugin::loadFromDefaultPaths();
}

WASMEDGE_CAPI_EXPORT void WasmEdge_PluginLoadFromPath(const char *Path) {
  WasmEdge::Plugin::Plugin::load(Path);
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_PluginListPluginsLength(void) {
  return static_cast<uint32_t>(WasmEdge::Plugin::Plugin::plugins().size());
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_PluginListPlugins(WasmEdge_String *Names,
                                                         const uint32_t Len) {
  auto PList = WasmEdge::Plugin::Plugin::plugins();
  if (Names) {
    for (uint32_t I = 0; I < Len && I < PList.size(); I++) {
      Names[I] = WasmEdge_String{
          /* Length */ static_cast<uint32_t>(std::strlen(PList[I].name())),
          /* Buf */ PList[I].name()};
    }
  }
  return static_cast<uint32_t>(PList.size());
}

WASMEDGE_CAPI_EXPORT const WasmEdge_PluginContext *
WasmEdge_PluginFind(const WasmEdge_String Name) {
  return toPluginCxt(WasmEdge::Plugin::Plugin::find(genStrView(Name)));
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_PluginGetPluginName(const WasmEdge_PluginContext *Cxt) {
  if (Cxt) {
    const char *Name = fromPluginCxt(Cxt)->name();
    return WasmEdge_String{
        /* Length */ static_cast<uint32_t>(std::strlen(Name)),
        /* Buf */ Name};
  }
  return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_PluginListModuleLength(const WasmEdge_PluginContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(fromPluginCxt(Cxt)->modules().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_PluginListModule(const WasmEdge_PluginContext *Cxt,
                          WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    auto MList = fromPluginCxt(Cxt)->modules();
    if (Names) {
      for (uint32_t I = 0; I < Len && I < MList.size(); I++) {
        Names[I] = WasmEdge_String{
            /* Length */ static_cast<uint32_t>(std::strlen(MList[I].name())),
            /* Buf */ MList[I].name()};
      }
    }
    return static_cast<uint32_t>(MList.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT WasmEdge_ModuleInstanceContext *
WasmEdge_PluginCreateModule(const WasmEdge_PluginContext *Cxt,
                            const WasmEdge_String ModuleName) {
  if (Cxt) {
    if (const auto *PMod =
            fromPluginCxt(Cxt)->findModule(genStrView(ModuleName));
        PMod) {
      return toModCxt(PMod->create().release());
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_PluginInitWASINN(const char *const *NNPreloads,
                          const uint32_t PreloadsLen) {
  using namespace std::literals::string_view_literals;
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_nn"sv)) {
    PO::ArgumentParser Parser;
    Plugin->registerOptions(Parser);
    Parser.set_raw_value<std::vector<std::string>>(
        "nn-preload"sv,
        std::vector<std::string>(NNPreloads, NNPreloads + PreloadsLen));
  }
}

// <<<<<<<< WasmEdge Plugin functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Experimental functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT void WasmEdge_ExecutorExperimentalRegisterPreHostFunction(
    WasmEdge_ExecutorContext *Cxt, void *Data, void (*Func)(void *)) {
  if (!Cxt) {
    return;
  }
  fromExecutorCxt(Cxt)->registerPreHostFunction(Data, Func);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ExecutorExperimentalRegisterPostHostFunction(
    WasmEdge_ExecutorContext *Cxt, void *Data, void (*Func)(void *)) {
  if (!Cxt) {
    return;
  }
  fromExecutorCxt(Cxt)->registerPostHostFunction(Data, Func);
}

// <<<<<<<< WasmEdge Experimental Functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#ifdef __cplusplus
} // extern "C"
#endif
