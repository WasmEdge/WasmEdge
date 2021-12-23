// SPDX-License-Identifier: Apache-2.0

#include "wasmedge/wasmedge.h"

#include "aot/compiler.h"
#include "host/wasi/wasimodule.h"
#include "host/wasmedge_process/processmodule.h"
#include "vm/vm.h"

#include <cstring>
#include <vector>

/// WasmEdge_ConfigureContext implementation.
struct WasmEdge_ConfigureContext {
  WasmEdge::Configure Conf;
};

/// WasmEdge_StatisticsContext implementation.
struct WasmEdge_StatisticsContext {};

/// WasmEdge_ASTModuleContext implementation.
struct WasmEdge_ASTModuleContext {
  WasmEdge_ASTModuleContext(std::unique_ptr<WasmEdge::AST::Module> Mod) noexcept
      : Module(std::move(Mod)) {}
  std::unique_ptr<WasmEdge::AST::Module> Module;
};

/// WasmEdge_FunctionTypeContext implementation.
struct WasmEdge_FunctionTypeContext {};

/// WasmEdge_TableTypeContext implementation.
struct WasmEdge_TableTypeContext {};

/// WasmEdge_MemoryTypeContext implementation.
struct WasmEdge_MemoryTypeContext {};

/// WasmEdge_GlobalTypeContext implementation.
struct WasmEdge_GlobalTypeContext {};

/// WasmEdge_ImportTypeContext implementation.
struct WasmEdge_ImportTypeContext {};

/// WasmEdge_ExportTypeContext implementation.
struct WasmEdge_ExportTypeContext {};

/// WasmEdge_CompilerContext implementation.
struct WasmEdge_CompilerContext {
#ifdef WASMEDGE_BUILD_AOT_RUNTIME
  WasmEdge_CompilerContext(const WasmEdge::Configure &Conf) noexcept
      : Compiler(Conf), Load(Conf), Valid(Conf) {}
  WasmEdge::AOT::Compiler Compiler;
  WasmEdge::Loader::Loader Load;
  WasmEdge::Validator::Validator Valid;
#endif
};

/// WasmEdge_LoaderContext implementation.
struct WasmEdge_LoaderContext {
  WasmEdge_LoaderContext(const WasmEdge::Configure &Conf) noexcept
      : Load(Conf) {}
  WasmEdge::Loader::Loader Load;
};

/// WasmEdge_ValidatorContext implementation.
struct WasmEdge_ValidatorContext {
  WasmEdge_ValidatorContext(const WasmEdge::Configure &Conf) noexcept
      : Valid(Conf) {}
  WasmEdge::Validator::Validator Valid;
};

/// WasmEdge_ExecutorContext implementation.
struct WasmEdge_ExecutorContext {
  WasmEdge_ExecutorContext(
      const WasmEdge::Configure &Conf,
      WasmEdge::Statistics::Statistics *S = nullptr) noexcept
      : Exec(Conf, S) {}
  WasmEdge::Executor::Executor Exec;
};

/// WasmEdge_StoreContext implementation.
struct WasmEdge_StoreContext {};

/// WasmEdge_FunctionInstanceContext implementation.
struct WasmEdge_FunctionInstanceContext {};

/// WasmEdge_TableInstanceContext implementation.
struct WasmEdge_TableInstanceContext {};

/// WasmEdge_MemoryInstanceContext implementation.
struct WasmEdge_MemoryInstanceContext {};

/// WasmEdge_GlobalInstanceContext implementation.
struct WasmEdge_GlobalInstanceContext {};

/// WasmEdge_ImportObjectContext implementation.
struct WasmEdge_ImportObjectContext {};

/// WasmEdge_Async implementation.
struct WasmEdge_Async {
  template <typename... Args>
  WasmEdge_Async(Args &&...Vals) noexcept
      : Async(std::forward<Args>(Vals)...) {}
  WasmEdge::VM::Async<WasmEdge::Expect<
      std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>>
      Async;
};

/// WasmEdge_VMContext implementation.
struct WasmEdge_VMContext {
  template <typename... Args>
  WasmEdge_VMContext(Args &&...Vals) noexcept
      : VM(std::forward<Args>(Vals)...) {}
  WasmEdge::VM::VM VM;
};

namespace {

using namespace WasmEdge;

/// Helper function for returning a WasmEdge_Result by error code.
inline constexpr WasmEdge_Result genWasmEdge_Result(ErrCode Code) noexcept {
  return WasmEdge_Result{.Code = static_cast<uint8_t>(Code)};
}

/// Helper function for returning a struct uint128_t / int128_t
/// from class WasmEdge::uint128_t / WasmEdge::int128_t.
template <typename C>
inline constexpr ::uint128_t to_uint128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__)
  return Val;
#else
  return {.Low = Val.low(), .High = static_cast<uint64_t>(Val.high())};
#endif
}
template <typename C> inline constexpr ::int128_t to_int128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__)
  return Val;
#else
  return {.Low = Val.low(), .High = Val.high()};
#endif
}

/// Helper function for returning a class WasmEdge::uint128_t /
/// WasmEdge::int128_t from struct uint128_t / int128_t.
template <typename C, typename T>
inline constexpr C to_WasmEdge_128_t(T Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__)
  return Val;
#else
  return C(Val.High, Val.Low);
#endif
}

/// Helper functions for returning a WasmEdge_Value by various values.
template <typename T> inline WasmEdge_Value genWasmEdge_Value(T Val) noexcept {
  return WasmEdge_Value{
      .Value = to_uint128_t(ValVariant(Val).unwrap()),
      .Type = static_cast<WasmEdge_ValType>(WasmEdge::ValTypeFromType<T>())};
}
inline WasmEdge_Value genWasmEdge_Value(ValVariant Val,
                                        WasmEdge_ValType T) noexcept {
  return WasmEdge_Value{.Value = to_uint128_t(Val.unwrap()), .Type = T};
}

/// Helper function for converting a WasmEdge_Value array to a ValVariant
/// vector.
inline std::pair<std::vector<ValVariant>, std::vector<ValType>>
genParamPair(const WasmEdge_Value *Val, const uint32_t Len) noexcept {
  std::vector<ValVariant> VVec;
  std::vector<ValType> TVec;
  if (Val == nullptr) {
    return {VVec, TVec};
  }
  VVec.resize(Len);
  TVec.resize(Len);
  for (uint32_t I = 0; I < Len; I++) {
    TVec[I] = static_cast<ValType>(Val[I].Type);
    switch (TVec[I]) {
    case ValType::I32:
      VVec[I] = ValVariant::wrap<uint32_t>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::I64:
      VVec[I] = ValVariant::wrap<uint64_t>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::F32:
      VVec[I] = ValVariant::wrap<float>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::F64:
      VVec[I] = ValVariant::wrap<double>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::V128:
      VVec[I] = ValVariant::wrap<WasmEdge::uint128_t>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::FuncRef:
      VVec[I] = ValVariant::wrap<FuncRef>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::ExternRef:
      VVec[I] = ValVariant::wrap<ExternRef>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Value));
      break;
    case ValType::None:
    default:
      // TODO: Return error
      __builtin_unreachable();
    }
  }
  return {VVec, TVec};
}

/// Helper function for making a Span to a uint8_t array.
template <typename T>
inline constexpr Span<const T> genSpan(const T *Buf,
                                       const uint32_t Len) noexcept {
  return Span<const T>(Buf, Len);
}

/// Helper functions for converting WasmEdge_String to std::String.
inline std::string_view genStrView(const WasmEdge_String S) noexcept {
  return std::string_view(S.Buf, S.Length);
}

/// Helper functions for converting a ValVariant vector to a WasmEdge_Value
/// array.
inline constexpr void
fillWasmEdge_ValueArr(Span<const std::pair<ValVariant, ValType>> Vec,
                      WasmEdge_Value *Val, const uint32_t Len) noexcept {
  if (Val == nullptr) {
    return;
  }
  for (uint32_t I = 0; I < Len && I < Vec.size(); I++) {
    Val[I] = genWasmEdge_Value(Vec[I].first,
                               static_cast<WasmEdge_ValType>(Vec[I].second));
  }
}

/// Helper template to run and return result.
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
      return genWasmEdge_Result(ErrCode::Success);
    } else {
      return genWasmEdge_Result(Res.error());
    }
  } else {
    return genWasmEdge_Result(ErrCode::WrongVMWorkflow);
  }
}

/// Helper function of retrieving exported maps.
inline uint32_t fillMap(const std::map<std::string, uint32_t, std::less<>> &Map,
                        WasmEdge_String *Names, const uint32_t Len) noexcept {
  uint32_t I = 0;
  for (auto &&Pair : Map) {
    if (I >= Len) {
      break;
    }
    if (Names) {
      Names[I] =
          WasmEdge_String{.Length = static_cast<uint32_t>(Pair.first.length()),
                          .Buf = Pair.first.data()};
    }
    I++;
  }
  return static_cast<uint32_t>(Map.size());
}

/// C API Host function class
class CAPIHostFunc : public Runtime::HostFunctionBase {
public:
  CAPIHostFunc(const AST::FunctionType *Type, WasmEdge_HostFunc_t FuncPtr,
               void *ExtData, const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(FuncPtr), Wrap(nullptr),
        Binding(nullptr), Data(ExtData) {
    FuncType = *Type;
  }
  CAPIHostFunc(const AST::FunctionType *Type, WasmEdge_WrapFunc_t WrapPtr,
               void *BindingPtr, void *ExtData,
               const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(nullptr), Wrap(WrapPtr),
        Binding(BindingPtr), Data(ExtData) {
    FuncType = *Type;
  }
  ~CAPIHostFunc() noexcept override = default;

  Expect<void> run(Runtime::Instance::MemoryInstance *MemInst,
                   Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {
    std::vector<WasmEdge_Value> Params(FuncType.getParamTypes().size()),
        Returns(FuncType.getReturnTypes().size());
    for (uint32_t I = 0; I < Args.size(); I++) {
      Params[I].Value = to_uint128_t(Args[I].get<WasmEdge::uint128_t>());
      Params[I].Type =
          static_cast<WasmEdge_ValType>(FuncType.getParamTypes()[I]);
    }
    WasmEdge_Value *PPtr = Params.size() ? (&Params[0]) : nullptr;
    WasmEdge_Value *RPtr = Returns.size() ? (&Returns[0]) : nullptr;
    auto *MemCxt = reinterpret_cast<WasmEdge_MemoryInstanceContext *>(MemInst);
    WasmEdge_Result Stat;
    if (Func) {
      Stat = Func(Data, MemCxt, PPtr, RPtr);
    } else {
      Stat = Wrap(Binding, Data, MemCxt, PPtr,
                  static_cast<uint32_t>(Params.size()), RPtr,
                  static_cast<uint32_t>(Returns.size()));
    }
    for (uint32_t I = 0; I < Rets.size(); I++) {
      Rets[I] = to_WasmEdge_128_t<WasmEdge::uint128_t>(Returns[I].Value);
    }
    if (!WasmEdge_ResultOK(Stat)) {
      return Unexpect(ErrCode::ExecutionFailed);
    } else if (Stat.Code == 0x01) {
      return Unexpect(ErrCode::Terminated);
    }
    return {};
  }

private:
  WasmEdge_HostFunc_t Func;
  WasmEdge_WrapFunc_t Wrap;
  void *Binding;
  void *Data;
};

/// Helper functions of context conversions.
#define CONVTO(SIMP, INST, NAME, QUANT)                                        \
  inline QUANT auto *to##SIMP##Cxt(QUANT INST *Cxt) noexcept {                 \
    return reinterpret_cast<QUANT WasmEdge_##NAME##Context *>(Cxt);            \
  }
CONVTO(Stat, Statistics::Statistics, Statistics, )
CONVTO(FuncType, AST::FunctionType, FunctionType, )
CONVTO(FuncType, AST::FunctionType, FunctionType, const)
CONVTO(TabType, AST::TableType, TableType, )
CONVTO(TabType, AST::TableType, TableType, const)
CONVTO(MemType, AST::MemoryType, MemoryType, )
CONVTO(MemType, AST::MemoryType, MemoryType, const)
CONVTO(GlobType, AST::GlobalType, GlobalType, )
CONVTO(GlobType, AST::GlobalType, GlobalType, const)
CONVTO(ImpType, AST::ImportDesc, ImportType, const)
CONVTO(ExpType, AST::ExportDesc, ExportType, const)
CONVTO(Store, Runtime::StoreManager, Store, )
CONVTO(Func, Runtime::Instance::FunctionInstance, FunctionInstance, )
CONVTO(Tab, Runtime::Instance::TableInstance, TableInstance, )
CONVTO(Mem, Runtime::Instance::MemoryInstance, MemoryInstance, )
CONVTO(Glob, Runtime::Instance::GlobalInstance, GlobalInstance, )
CONVTO(ImpObj, Runtime::ImportObject, ImportObject, )
#undef CONVTO

#define CONVFROM(SIMP, INST, NAME, QUANT)                                      \
  inline QUANT auto *from##SIMP##Cxt(                                          \
      QUANT WasmEdge_##NAME##Context *Cxt) noexcept {                          \
    return reinterpret_cast<QUANT INST *>(Cxt);                                \
  }
CONVFROM(Stat, Statistics::Statistics, Statistics, )
CONVFROM(Stat, Statistics::Statistics, Statistics, const)
CONVFROM(FuncType, AST::FunctionType, FunctionType, )
CONVFROM(FuncType, AST::FunctionType, FunctionType, const)
CONVFROM(TabType, AST::TableType, TableType, )
CONVFROM(TabType, AST::TableType, TableType, const)
CONVFROM(MemType, AST::MemoryType, MemoryType, )
CONVFROM(MemType, AST::MemoryType, MemoryType, const)
CONVFROM(GlobType, AST::GlobalType, GlobalType, )
CONVFROM(GlobType, AST::GlobalType, GlobalType, const)
CONVFROM(ImpType, AST::ImportDesc, ImportType, const)
CONVFROM(ExpType, AST::ExportDesc, ExportType, const)
CONVFROM(Store, Runtime::StoreManager, Store, )
CONVFROM(Store, Runtime::StoreManager, Store, const)
CONVFROM(Func, Runtime::Instance::FunctionInstance, FunctionInstance, )
CONVFROM(Func, Runtime::Instance::FunctionInstance, FunctionInstance, const)
CONVFROM(Tab, Runtime::Instance::TableInstance, TableInstance, )
CONVFROM(Tab, Runtime::Instance::TableInstance, TableInstance, const)
CONVFROM(Mem, Runtime::Instance::MemoryInstance, MemoryInstance, )
CONVFROM(Mem, Runtime::Instance::MemoryInstance, MemoryInstance, const)
CONVFROM(Glob, Runtime::Instance::GlobalInstance, GlobalInstance, )
CONVFROM(Glob, Runtime::Instance::GlobalInstance, GlobalInstance, const)
CONVFROM(ImpObj, Runtime::ImportObject, ImportObject, )
CONVFROM(ImpObj, Runtime::ImportObject, ImportObject, const)
#undef CONVFROM

} // namespace

#ifdef __cplusplus
extern "C" {
#endif

/// >>>>>>>> WasmEdge version functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

/// <<<<<<<< WasmEdge version functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge logging functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT void WasmEdge_LogSetErrorLevel(void) {
  WasmEdge::Log::setErrorLoggingLevel();
}

WASMEDGE_CAPI_EXPORT void WasmEdge_LogSetDebugLevel(void) {
  WasmEdge::Log::setDebugLoggingLevel();
}

/// <<<<<<<< WasmEdge logging functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge value functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
WasmEdge_ValueGenNullRef(const WasmEdge_RefType T) {
  return genWasmEdge_Value(WasmEdge::UnknownRef(),
                           static_cast<WasmEdge_ValType>(T));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value
WasmEdge_ValueGenFuncRef(const uint32_t Index) {
  return genWasmEdge_Value(WasmEdge::FuncRef(Index), WasmEdge_ValType_FuncRef);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenExternRef(void *Ref) {
  return genWasmEdge_Value(WasmEdge::ExternRef(Ref),
                           WasmEdge_ValType_ExternRef);
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
  return WasmEdge::isNullRef(WasmEdge::ValVariant::wrap<UnknownRef>(
      to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value)));
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ValueGetFuncIdx(const WasmEdge_Value Val) {
  return WasmEdge::retrieveFuncIdx(WasmEdge::ValVariant::wrap<FuncRef>(
      to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value)));
}

WASMEDGE_CAPI_EXPORT void *
WasmEdge_ValueGetExternRef(const WasmEdge_Value Val) {
  return &WasmEdge::retrieveExternRef<uint32_t>(
      WasmEdge::ValVariant::wrap<ExternRef>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val.Value)));
}

/// <<<<<<<< WasmEdge value functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// <<<<<<<< WasmEdge string functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_StringCreateByCString(const char *Str) {
  if (Str) {
    return WasmEdge_StringCreateByBuffer(
        Str, static_cast<uint32_t>(std::strlen(Str)));
  }
  return WasmEdge_String{.Length = 0, .Buf = nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_StringCreateByBuffer(const char *Buf, const uint32_t Len) {
  if (Buf && Len) {
    char *Str = new char[Len];
    std::copy_n(Buf, Len, Str);
    return WasmEdge_String{.Length = Len, .Buf = Str};
  }
  return WasmEdge_String{.Length = 0, .Buf = nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_String WasmEdge_StringWrap(const char *Buf,
                                                         const uint32_t Len) {
  return WasmEdge_String{.Length = Len, .Buf = Buf};
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

/// >>>>>>>> WasmEdge string functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// >>>>>>>> WasmEdge result functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT bool WasmEdge_ResultOK(const WasmEdge_Result Res) {
  if (static_cast<WasmEdge::ErrCode>(Res.Code) == WasmEdge::ErrCode::Success ||
      static_cast<WasmEdge::ErrCode>(Res.Code) ==
          WasmEdge::ErrCode::Terminated) {
    return true;
  } else {
    return false;
  }
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ResultGetCode(const WasmEdge_Result Res) {
  return static_cast<uint32_t>(Res.Code);
}

WASMEDGE_CAPI_EXPORT const char *
WasmEdge_ResultGetMessage(const WasmEdge_Result Res) {
  return WasmEdge::ErrCodeStr[static_cast<WasmEdge::ErrCode>(Res.Code)].c_str();
}

/// <<<<<<<< WasmEdge result functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge limit functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT bool WasmEdge_LimitIsEqual(const WasmEdge_Limit Lim1,
                                                const WasmEdge_Limit Lim2) {
  return Lim1.HasMax == Lim2.HasMax && Lim1.Min == Lim2.Min &&
         Lim1.Max == Lim2.Max;
}

/// <<<<<<<< WasmEdge limit functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge configure functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ConfigureContext *WasmEdge_ConfigureCreate(void) {
  return new WasmEdge_ConfigureContext;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureAddProposal(WasmEdge_ConfigureContext *Cxt,
                              const WasmEdge_Proposal Prop) {
  if (Cxt) {
    Cxt->Conf.addProposal(static_cast<WasmEdge::Proposal>(Prop));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureRemoveProposal(WasmEdge_ConfigureContext *Cxt,
                                 const WasmEdge_Proposal Prop) {
  if (Cxt) {
    Cxt->Conf.removeProposal(static_cast<WasmEdge::Proposal>(Prop));
  }
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ConfigureHasProposal(const WasmEdge_ConfigureContext *Cxt,
                              const WasmEdge_Proposal Prop) {
  if (Cxt) {
    return Cxt->Conf.hasProposal(static_cast<WasmEdge::Proposal>(Prop));
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureAddHostRegistration(WasmEdge_ConfigureContext *Cxt,
                                      const WasmEdge_HostRegistration Host) {
  if (Cxt) {
    Cxt->Conf.addHostRegistration(
        static_cast<WasmEdge::HostRegistration>(Host));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureRemoveHostRegistration(WasmEdge_ConfigureContext *Cxt,
                                         const WasmEdge_HostRegistration Host) {
  if (Cxt) {
    Cxt->Conf.removeHostRegistration(
        static_cast<WasmEdge::HostRegistration>(Host));
  }
}

WASMEDGE_CAPI_EXPORT bool
WasmEdge_ConfigureHasHostRegistration(const WasmEdge_ConfigureContext *Cxt,
                                      const WasmEdge_HostRegistration Host) {
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

/// <<<<<<<< WasmEdge configure functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge statistics functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
WasmEdge_StatisticsDelete(WasmEdge_StatisticsContext *Cxt) {
  delete fromStatCxt(Cxt);
}

/// <<<<<<<< WasmEdge statistics functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge AST module functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ASTModuleListImportsLength(const WasmEdge_ASTModuleContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(
        Cxt->Module->getImportSection().getContent().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ASTModuleListImports(
    const WasmEdge_ASTModuleContext *Cxt,
    const WasmEdge_ImportTypeContext **Imports, const uint32_t Len) {
  if (Cxt) {
    const auto &ImpSec = Cxt->Module->getImportSection().getContent();
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
        Cxt->Module->getExportSection().getContent().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_ASTModuleListExports(
    const WasmEdge_ASTModuleContext *Cxt,
    const WasmEdge_ExportTypeContext **Exports, const uint32_t Len) {
  if (Cxt) {
    const auto &ExpSec = Cxt->Module->getExportSection().getContent();
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
  delete Cxt;
}

/// <<<<<<<< WasmEdge AST module functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge function type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionTypeContext *WasmEdge_FunctionTypeCreate(
    const enum WasmEdge_ValType *ParamList, const uint32_t ParamLen,
    const enum WasmEdge_ValType *ReturnList, const uint32_t ReturnLen) {
  auto *Cxt = new WasmEdge::AST::FunctionType;
  if (ParamLen > 0) {
    Cxt->getParamTypes().resize(ParamLen);
  }
  for (uint32_t I = 0; I < ParamLen; I++) {
    Cxt->getParamTypes()[I] = static_cast<WasmEdge::ValType>(ParamList[I]);
  }
  if (ReturnLen > 0) {
    Cxt->getReturnTypes().resize(ReturnLen);
  }
  for (uint32_t I = 0; I < ReturnLen; I++) {
    Cxt->getReturnTypes()[I] = static_cast<WasmEdge::ValType>(ReturnList[I]);
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
      List[I] = static_cast<WasmEdge_ValType>(
          fromFuncTypeCxt(Cxt)->getParamTypes()[I]);
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
      List[I] = static_cast<WasmEdge_ValType>(
          fromFuncTypeCxt(Cxt)->getReturnTypes()[I]);
    }
    return static_cast<uint32_t>(fromFuncTypeCxt(Cxt)->getReturnTypes().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_FunctionTypeDelete(WasmEdge_FunctionTypeContext *Cxt) {
  delete fromFuncTypeCxt(Cxt);
}

/// <<<<<<<< WasmEdge function type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge table type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_TableTypeContext *
WasmEdge_TableTypeCreate(const enum WasmEdge_RefType RefType,
                         const WasmEdge_Limit Limit) {
  WasmEdge::RefType Type = static_cast<WasmEdge::RefType>(RefType);
  if (Limit.HasMax) {
    return toTabTypeCxt(
        new WasmEdge::AST::TableType(Type, Limit.Min, Limit.Max));
  } else {
    return toTabTypeCxt(new WasmEdge::AST::TableType(Type, Limit.Min));
  }
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_RefType
WasmEdge_TableTypeGetRefType(const WasmEdge_TableTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_RefType>(fromTabTypeCxt(Cxt)->getRefType());
  }
  return WasmEdge_RefType_FuncRef;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Limit
WasmEdge_TableTypeGetLimit(const WasmEdge_TableTypeContext *Cxt) {
  if (Cxt) {
    const auto &Lim = fromTabTypeCxt(Cxt)->getLimit();
    return WasmEdge_Limit{
        .HasMax = Lim.hasMax(), .Min = Lim.getMin(), .Max = Lim.getMax()};
  }
  return WasmEdge_Limit{.HasMax = false, .Min = 0, .Max = 0};
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_TableTypeDelete(WasmEdge_TableTypeContext *Cxt) {
  delete fromTabTypeCxt(Cxt);
}

/// <<<<<<<< WasmEdge table type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge memory type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryTypeContext *
WasmEdge_MemoryTypeCreate(const WasmEdge_Limit Limit) {
  if (Limit.HasMax) {
    return toMemTypeCxt(new WasmEdge::AST::MemoryType(Limit.Min, Limit.Max));
  } else {
    return toMemTypeCxt(new WasmEdge::AST::MemoryType(Limit.Min));
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Limit
WasmEdge_MemoryTypeGetLimit(const WasmEdge_MemoryTypeContext *Cxt) {
  if (Cxt) {
    const auto &Lim = fromMemTypeCxt(Cxt)->getLimit();
    return WasmEdge_Limit{
        .HasMax = Lim.hasMax(), .Min = Lim.getMin(), .Max = Lim.getMax()};
  }
  return WasmEdge_Limit{.HasMax = false, .Min = 0, .Max = 0};
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_MemoryTypeDelete(WasmEdge_MemoryTypeContext *Cxt) {
  delete fromMemTypeCxt(Cxt);
}

/// <<<<<<<< WasmEdge memory type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge global type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalTypeContext *
WasmEdge_GlobalTypeCreate(const enum WasmEdge_ValType ValType,
                          const enum WasmEdge_Mutability Mut) {
  return toGlobTypeCxt(
      new WasmEdge::AST::GlobalType(static_cast<WasmEdge::ValType>(ValType),
                                    static_cast<WasmEdge::ValMut>(Mut)));
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_ValType
WasmEdge_GlobalTypeGetValType(const WasmEdge_GlobalTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_ValType>(fromGlobTypeCxt(Cxt)->getValType());
  }
  return WasmEdge_ValType_I32;
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

/// <<<<<<<< WasmEdge global type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge import type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
    return WasmEdge_String{.Length = static_cast<uint32_t>(StrView.length()),
                           .Buf = StrView.data()};
  }
  return WasmEdge_String{.Length = 0, .Buf = nullptr};
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_ImportTypeGetExternalName(const WasmEdge_ImportTypeContext *Cxt) {
  if (Cxt) {
    auto StrView = fromImpTypeCxt(Cxt)->getExternalName();
    return WasmEdge_String{.Length = static_cast<uint32_t>(StrView.length()),
                           .Buf = StrView.data()};
  }
  return WasmEdge_String{.Length = 0, .Buf = nullptr};
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_ImportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt,
                                   const WasmEdge_ImportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromImpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Function) {
    uint32_t Idx = fromImpTypeCxt(Cxt)->getExternalFuncTypeIdx();
    const auto &FuncTypes = ASTCxt->Module->getTypeSection().getContent();
    if (Idx >= FuncTypes.size()) {
      return nullptr;
    }
    return toFuncTypeCxt(&FuncTypes[Idx]);
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

/// <<<<<<<< WasmEdge import type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge export type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
    return WasmEdge_String{.Length = static_cast<uint32_t>(StrView.length()),
                           .Buf = StrView.data()};
  }
  return WasmEdge_String{.Length = 0, .Buf = nullptr};
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_ExportTypeGetFunctionType(const WasmEdge_ASTModuleContext *ASTCxt,
                                   const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Function) {
    /// `external_index` = `func_index` + `import_func_nums`
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();
    const auto &ImpDescs = ASTCxt->Module->getImportSection().getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Function) {
        ExtIdx--;
      }
    }
    /// Get the function type index by the function index
    const auto &FuncIdxs = ASTCxt->Module->getFunctionSection().getContent();
    if (ExtIdx >= FuncIdxs.size()) {
      return nullptr;
    }
    uint32_t TypeIdx = FuncIdxs[ExtIdx];
    /// Get the function type
    const auto &FuncTypes = ASTCxt->Module->getTypeSection().getContent();
    if (TypeIdx >= FuncTypes.size()) {
      return nullptr;
    }
    return toFuncTypeCxt(&FuncTypes[TypeIdx]);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_TableTypeContext *
WasmEdge_ExportTypeGetTableType(const WasmEdge_ASTModuleContext *ASTCxt,
                                const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() == WasmEdge::ExternalType::Table) {
    /// `external_index` = `table_type_index` + `import_table_nums`
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();
    const auto &ImpDescs = ASTCxt->Module->getImportSection().getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Table) {
        ExtIdx--;
      }
    }
    /// Get the table type
    const auto &TabTypes = ASTCxt->Module->getTableSection().getContent();
    if (ExtIdx >= TabTypes.size()) {
      return nullptr;
    }
    return toTabTypeCxt(&TabTypes[ExtIdx]);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_MemoryTypeContext *
WasmEdge_ExportTypeGetMemoryType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Memory) {
    /// `external_index` = `memory_type_index` + `import_memory_nums`
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();
    const auto &ImpDescs = ASTCxt->Module->getImportSection().getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Memory) {
        ExtIdx--;
      }
    }
    /// Get the memory type
    const auto &MemTypes = ASTCxt->Module->getMemorySection().getContent();
    if (ExtIdx >= MemTypes.size()) {
      return nullptr;
    }
    return toMemTypeCxt(&MemTypes[ExtIdx]);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const WasmEdge_GlobalTypeContext *
WasmEdge_ExportTypeGetGlobalType(const WasmEdge_ASTModuleContext *ASTCxt,
                                 const WasmEdge_ExportTypeContext *Cxt) {
  if (ASTCxt && Cxt &&
      fromExpTypeCxt(Cxt)->getExternalType() ==
          WasmEdge::ExternalType::Global) {
    /// `external_index` = `global_type_index` + `import_global_nums`
    uint32_t ExtIdx = fromExpTypeCxt(Cxt)->getExternalIndex();
    const auto &ImpDescs = ASTCxt->Module->getImportSection().getContent();
    for (auto &&ImpDesc : ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Global) {
        ExtIdx--;
      }
    }
    /// Get the global type
    const auto &GlobDescs = ASTCxt->Module->getGlobalSection().getContent();
    if (ExtIdx >= GlobDescs.size()) {
      return nullptr;
    }
    return toGlobTypeCxt(&GlobDescs[ExtIdx].getGlobalType());
  }
  return nullptr;
}

/// <<<<<<<< WasmEdge export type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge AOT compiler functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_CompilerContext *
WasmEdge_CompilerCreate(const WasmEdge_ConfigureContext *ConfCxt
                        [[maybe_unused]]) {
#ifdef WASMEDGE_BUILD_AOT_RUNTIME
  if (ConfCxt) {
    return new WasmEdge_CompilerContext(ConfCxt->Conf);
  } else {
    return new WasmEdge_CompilerContext(WasmEdge::Configure());
  }
#else
  return nullptr;
#endif
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_CompilerCompile(
    WasmEdge_CompilerContext *Cxt [[maybe_unused]],
    const char *InPath [[maybe_unused]], const char *OutPath [[maybe_unused]]) {
#ifdef WASMEDGE_BUILD_AOT_RUNTIME
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        std::filesystem::path InputPath = std::filesystem::absolute(InPath);
        std::filesystem::path OutputPath = std::filesystem::absolute(OutPath);
        std::vector<WasmEdge::Byte> Data;
        std::unique_ptr<WasmEdge::AST::Module> Module;
        if (auto Res = Cxt->Load.loadFile(InputPath)) {
          Data = std::move(*Res);
        } else {
          return Unexpect(Res);
        }
        if (auto Res = Cxt->Load.parseModule(Data)) {
          Module = std::move(*Res);
        } else {
          return Unexpect(Res);
        }
        if (auto Res = Cxt->Valid.validate(*Module); !Res) {
          return Unexpect(Res);
        }
        return Cxt->Compiler.compile(Data, *Module, OutputPath);
      },
      EmptyThen, Cxt);
#else
  return WasmEdge_Result{
      .Code = static_cast<uint8_t>(WasmEdge::ErrCode::AOTDisabled)};
#endif
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_CompilerDelete(WasmEdge_CompilerContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge AOT compiler functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge loader functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_LoaderContext *
WasmEdge_LoaderCreate(const WasmEdge_ConfigureContext *ConfCxt) {
  if (ConfCxt) {
    return new WasmEdge_LoaderContext(ConfCxt->Conf);
  } else {
    return new WasmEdge_LoaderContext(WasmEdge::Configure());
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_LoaderParseFromFile(
    WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module,
    const char *Path) {
  return wrap(
      [&]() { return Cxt->Load.parseModule(std::filesystem::absolute(Path)); },
      [&](auto &&Res) {
        *Module = new WasmEdge_ASTModuleContext(std::move(*Res));
      },
      Cxt, Module);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_LoaderParseFromBuffer(
    WasmEdge_LoaderContext *Cxt, WasmEdge_ASTModuleContext **Module,
    const uint8_t *Buf, const uint32_t BufLen) {
  return wrap([&]() { return Cxt->Load.parseModule(genSpan(Buf, BufLen)); },
              [&](auto &&Res) {
                *Module = new WasmEdge_ASTModuleContext(std::move(*Res));
              },
              Cxt, Module);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_LoaderDelete(WasmEdge_LoaderContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge loader functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge validator functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ValidatorContext *
WasmEdge_ValidatorCreate(const WasmEdge_ConfigureContext *ConfCxt) {
  if (ConfCxt) {
    return new WasmEdge_ValidatorContext(ConfCxt->Conf);
  } else {
    return new WasmEdge_ValidatorContext(WasmEdge::Configure());
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_ValidatorValidate(WasmEdge_ValidatorContext *Cxt,
                           const WasmEdge_ASTModuleContext *ModuleCxt) {
  return wrap([&]() { return Cxt->Valid.validate(*ModuleCxt->Module.get()); },
              EmptyThen, Cxt, ModuleCxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ValidatorDelete(WasmEdge_ValidatorContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge validator functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge executor functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_ExecutorContext *
WasmEdge_ExecutorCreate(const WasmEdge_ConfigureContext *ConfCxt,
                        WasmEdge_StatisticsContext *StatCxt) {
  if (ConfCxt) {
    if (StatCxt) {
      return new WasmEdge_ExecutorContext(ConfCxt->Conf, fromStatCxt(StatCxt));
    } else {
      return new WasmEdge_ExecutorContext(ConfCxt->Conf);
    }
  } else {
    if (StatCxt) {
      return new WasmEdge_ExecutorContext(WasmEdge::Configure(),
                                          fromStatCxt(StatCxt));
    } else {
      return new WasmEdge_ExecutorContext(WasmEdge::Configure());
    }
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorInstantiate(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->Exec.instantiateModule(*fromStoreCxt(StoreCxt),
                                           *ASTCxt->Module.get());
      },
      EmptyThen, Cxt, StoreCxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorRegisterImport(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ImportObjectContext *ImportCxt) {
  return wrap(
      [&]() {
        return Cxt->Exec.registerModule(*fromStoreCxt(StoreCxt),
                                        *fromImpObjCxt(ImportCxt));
      },
      EmptyThen, Cxt, StoreCxt, ImportCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorRegisterModule(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_String ModuleName) {
  return wrap(
      [&]() {
        return Cxt->Exec.registerModule(*fromStoreCxt(StoreCxt),
                                        *ASTCxt->Module.get(),
                                        genStrView(ModuleName));
      },
      EmptyThen, Cxt, StoreCxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorInvoke(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns,
    const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  auto FuncStr = genStrView(FuncName);
  return wrap(
      [&]()
          -> WasmEdge::Expect<
              std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>> {
        /// Get module instance.
        WasmEdge::Runtime::Instance::ModuleInstance *ModInst;
        if (auto Res = fromStoreCxt(StoreCxt)->getActiveModule()) {
          ModInst = *Res;
        } else {
          spdlog::error(Res.error());
          spdlog::error(WasmEdge::ErrInfo::InfoExecuting("", FuncStr));
          return Unexpect(Res);
        }

        /// Check exports for finding function address.
        const auto &FuncExp = ModInst->getFuncExports();
        const auto FuncIter = FuncExp.find(genStrView(FuncName));
        if (FuncIter == FuncExp.cend()) {
          spdlog::error(WasmEdge::ErrCode::FuncNotFound);
          spdlog::error(WasmEdge::ErrInfo::InfoExecuting("", FuncStr));
          return Unexpect(WasmEdge::ErrCode::FuncNotFound);
        }
        return Cxt->Exec.invoke(*fromStoreCxt(StoreCxt), FuncIter->second,
                                ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      StoreCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_ExecutorInvokeRegistered(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_String ModuleName, const WasmEdge_String FuncName,
    const WasmEdge_Value *Params, const uint32_t ParamLen,
    WasmEdge_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  auto ModStr = genStrView(ModuleName);
  auto FuncStr = genStrView(FuncName);
  return wrap(
      [&]()
          -> WasmEdge::Expect<
              std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>> {
        /// Get module instance.
        WasmEdge::Runtime::Instance::ModuleInstance *ModInst;
        if (auto Res = fromStoreCxt(StoreCxt)->findModule(ModStr)) {
          ModInst = *Res;
        } else {
          spdlog::error(Res.error());
          spdlog::error(WasmEdge::ErrInfo::InfoExecuting(ModStr, FuncStr));
          return Unexpect(Res);
        }

        /// Get exports and find function.
        const auto &FuncExp = ModInst->getFuncExports();
        const auto FuncIter = FuncExp.find(FuncStr);
        if (FuncIter == FuncExp.cend()) {
          spdlog::error(WasmEdge::ErrCode::FuncNotFound);
          spdlog::error(WasmEdge::ErrInfo::InfoExecuting(ModStr, FuncStr));
          return Unexpect(WasmEdge::ErrCode::FuncNotFound);
        }
        return Cxt->Exec.invoke(*fromStoreCxt(StoreCxt), FuncIter->second,
                                ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      StoreCxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ExecutorDelete(WasmEdge_ExecutorContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge executor functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_StoreContext *WasmEdge_StoreCreate(void) {
  return toStoreCxt(new WasmEdge::Runtime::StoreManager);
}

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionInstanceContext *
WasmEdge_StoreFindFunction(WasmEdge_StoreContext *Cxt,
                           const WasmEdge_String Name) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      const auto &FuncExp = (*Res)->getFuncExports();
      const auto FuncIter = FuncExp.find(genStrView(Name));
      if (FuncIter != FuncExp.cend()) {
        return toFuncCxt(*fromStoreCxt(Cxt)->getFunction(FuncIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionInstanceContext *
WasmEdge_StoreFindFunctionRegistered(WasmEdge_StoreContext *Cxt,
                                     const WasmEdge_String ModuleName,
                                     const WasmEdge_String FuncName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      const auto &FuncExp = (*Res)->getFuncExports();
      const auto FuncIter = FuncExp.find(genStrView(FuncName));
      if (FuncIter != FuncExp.cend()) {
        return toFuncCxt(*fromStoreCxt(Cxt)->getFunction(FuncIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_TableInstanceContext *
WasmEdge_StoreFindTable(WasmEdge_StoreContext *Cxt,
                        const WasmEdge_String Name) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      const auto &TabExp = (*Res)->getTableExports();
      const auto TabIter = TabExp.find(genStrView(Name));
      if (TabIter != TabExp.cend()) {
        return toTabCxt(*fromStoreCxt(Cxt)->getTable(TabIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_TableInstanceContext *
WasmEdge_StoreFindTableRegistered(WasmEdge_StoreContext *Cxt,
                                  const WasmEdge_String ModuleName,
                                  const WasmEdge_String TableName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      const auto &TabExp = (*Res)->getTableExports();
      const auto TabIter = TabExp.find(genStrView(TableName));
      if (TabIter != TabExp.cend()) {
        return toTabCxt(*fromStoreCxt(Cxt)->getTable(TabIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryInstanceContext *
WasmEdge_StoreFindMemory(WasmEdge_StoreContext *Cxt,
                         const WasmEdge_String Name) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      const auto &MemExp = (*Res)->getMemExports();
      const auto MemIter = MemExp.find(genStrView(Name));
      if (MemIter != MemExp.cend()) {
        return toMemCxt(*fromStoreCxt(Cxt)->getMemory(MemIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_MemoryInstanceContext *
WasmEdge_StoreFindMemoryRegistered(WasmEdge_StoreContext *Cxt,
                                   const WasmEdge_String ModuleName,
                                   const WasmEdge_String MemoryName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      const auto &MemExp = (*Res)->getMemExports();
      const auto MemIter = MemExp.find(genStrView(MemoryName));
      if (MemIter != MemExp.cend()) {
        return toMemCxt(*fromStoreCxt(Cxt)->getMemory(MemIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalInstanceContext *
WasmEdge_StoreFindGlobal(WasmEdge_StoreContext *Cxt,
                         const WasmEdge_String Name) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      const auto &GlobExp = (*Res)->getGlobalExports();
      const auto GlobIter = GlobExp.find(genStrView(Name));
      if (GlobIter != GlobExp.cend()) {
        return toGlobCxt(*fromStoreCxt(Cxt)->getGlobal(GlobIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalInstanceContext *
WasmEdge_StoreFindGlobalRegistered(WasmEdge_StoreContext *Cxt,
                                   const WasmEdge_String ModuleName,
                                   const WasmEdge_String GlobalName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      const auto &GlobExp = (*Res)->getGlobalExports();
      const auto GlobIter = GlobExp.find(genStrView(GlobalName));
      if (GlobIter != GlobExp.cend()) {
        return toGlobCxt(*fromStoreCxt(Cxt)->getGlobal(GlobIter->second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListFunctionLength(const WasmEdge_StoreContext *Cxt) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return static_cast<uint32_t>((*Res)->getFuncExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListFunction(const WasmEdge_StoreContext *Cxt,
                           WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return fillMap((*Res)->getFuncExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListFunctionRegisteredLength(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return static_cast<uint32_t>((*Res)->getFuncExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListFunctionRegistered(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
    WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getFuncExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListTableLength(const WasmEdge_StoreContext *Cxt) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return static_cast<uint32_t>((*Res)->getTableExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListTable(const WasmEdge_StoreContext *Cxt,
                        WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return fillMap((*Res)->getTableExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListTableRegisteredLength(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return static_cast<uint32_t>((*Res)->getTableExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListTableRegistered(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
    WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getTableExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListMemoryLength(const WasmEdge_StoreContext *Cxt) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return static_cast<uint32_t>((*Res)->getMemExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListMemory(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return fillMap((*Res)->getMemExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListMemoryRegisteredLength(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return static_cast<uint32_t>((*Res)->getMemExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListMemoryRegistered(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
    WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getMemExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListGlobalLength(const WasmEdge_StoreContext *Cxt) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return static_cast<uint32_t>((*Res)->getGlobalExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListGlobal(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->getActiveModule()) {
      return fillMap((*Res)->getGlobalExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListGlobalRegisteredLength(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return static_cast<uint32_t>((*Res)->getGlobalExports().size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListGlobalRegistered(
    const WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
    WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getGlobalExports(), Names, Len);
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListModuleLength(const WasmEdge_StoreContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(fromStoreCxt(Cxt)->getModuleList().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListModule(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getModuleList(), Names, Len);
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_StoreDelete(WasmEdge_StoreContext *Cxt) {
  delete fromStoreCxt(Cxt);
}

/// <<<<<<<< WasmEdge store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge function instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

WASMEDGE_CAPI_EXPORT void
WasmEdge_FunctionInstanceDelete(WasmEdge_FunctionInstanceContext *Cxt) {
  delete fromFuncCxt(Cxt);
}

/// <<<<<<<< WasmEdge function instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge table instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_TableInstanceContext *
WasmEdge_TableInstanceCreate(const WasmEdge_TableTypeContext *TabType) {
  if (TabType) {
    return toTabCxt(new WasmEdge::Runtime::Instance::TableInstance(
        *fromTabTypeCxt(TabType)));
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
              [&](auto &&Res) {
                *Data = genWasmEdge_Value(
                    Res->template get<UnknownRef>(),
                    static_cast<WasmEdge_ValType>(
                        fromTabCxt(Cxt)->getTableType().getRefType()));
              },
              Cxt, Data);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_TableInstanceSetData(WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value Data, const uint32_t Offset) {
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        WasmEdge::RefType expType =
            fromTabCxt(Cxt)->getTableType().getRefType();
        if (expType != static_cast<WasmEdge::RefType>(Data.Type)) {
          spdlog::error(WasmEdge::ErrCode::RefTypeMismatch);
          spdlog::error(WasmEdge::ErrInfo::InfoMismatch(
              static_cast<WasmEdge::ValType>(expType),
              static_cast<WasmEdge::ValType>(Data.Type)));
          return Unexpect(WasmEdge::ErrCode::RefTypeMismatch);
        }
        return fromTabCxt(Cxt)->setRefAddr(
            Offset, WasmEdge::ValVariant(
                        to_WasmEdge_128_t<WasmEdge::uint128_t>(Data.Value))
                        .get<UnknownRef>());
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
          return WasmEdge::Unexpect(WasmEdge::ErrCode::TableOutOfBounds);
        }
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_TableInstanceDelete(WasmEdge_TableInstanceContext *Cxt) {
  delete fromTabCxt(Cxt);
}

/// <<<<<<<< WasmEdge table instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge memory instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
    return fromMemCxt(Cxt)->getPointer<uint8_t *>(Offset, Length);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT const uint8_t *WasmEdge_MemoryInstanceGetPointerConst(
    const WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Offset,
    const uint32_t Length) {
  if (Cxt) {
    return fromMemCxt(Cxt)->getPointer<const uint8_t *>(Offset, Length);
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
          return WasmEdge::Unexpect(WasmEdge::ErrCode::MemoryOutOfBounds);
        }
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_MemoryInstanceDelete(WasmEdge_MemoryInstanceContext *Cxt) {
  delete fromMemCxt(Cxt);
}

/// <<<<<<<< WasmEdge memory instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge global instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_GlobalInstanceContext *
WasmEdge_GlobalInstanceCreate(const WasmEdge_GlobalTypeContext *GlobType,
                              const WasmEdge_Value Value) {
  if (GlobType && Value.Type == static_cast<WasmEdge_ValType>(
                                    fromGlobTypeCxt(GlobType)->getValType())) {
    return toGlobCxt(new WasmEdge::Runtime::Instance::GlobalInstance(
        *fromGlobTypeCxt(GlobType),
        to_WasmEdge_128_t<WasmEdge::uint128_t>(Value.Value)));
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
    return genWasmEdge_Value(
        fromGlobCxt(Cxt)->getValue(),
        static_cast<WasmEdge_ValType>(
            fromGlobCxt(Cxt)->getGlobalType().getValType()));
  }
  return genWasmEdge_Value(
      WasmEdge::ValVariant(static_cast<WasmEdge::uint128_t>(0)),
      WasmEdge_ValType_I32);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_GlobalInstanceSetValue(WasmEdge_GlobalInstanceContext *Cxt,
                                const WasmEdge_Value Value) {
  if (Cxt &&
      fromGlobCxt(Cxt)->getGlobalType().getValMut() == WasmEdge::ValMut::Var &&
      static_cast<WasmEdge::ValType>(Value.Type) ==
          fromGlobCxt(Cxt)->getGlobalType().getValType()) {
    fromGlobCxt(Cxt)->getValue() =
        to_WasmEdge_128_t<WasmEdge::uint128_t>(Value.Value);
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_GlobalInstanceDelete(WasmEdge_GlobalInstanceContext *Cxt) {
  delete fromGlobCxt(Cxt);
}

/// <<<<<<<< WasmEdge global instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// <<<<<<<< WasmEdge import object functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

WASMEDGE_CAPI_EXPORT WasmEdge_ImportObjectContext *
WasmEdge_ImportObjectCreate(const WasmEdge_String ModuleName) {
  return toImpObjCxt(
      new WasmEdge::Runtime::ImportObject(genStrView(ModuleName)));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ImportObjectContext *
WasmEdge_ImportObjectCreateWASI(const char *const *Args, const uint32_t ArgLen,
                                const char *const *Envs, const uint32_t EnvLen,
                                const char *const *Preopens,
                                const uint32_t PreopenLen) {
  auto *WasiMod = new WasmEdge::Host::WasiModule();
  WasmEdge_ImportObjectInitWASI(toImpObjCxt(WasiMod), Args, ArgLen, Envs,
                                EnvLen, Preopens, PreopenLen);
  return toImpObjCxt(WasiMod);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ImportObjectInitWASI(
    WasmEdge_ImportObjectContext *Cxt, const char *const *Args,
    const uint32_t ArgLen, const char *const *Envs, const uint32_t EnvLen,
    const char *const *Preopens, const uint32_t PreopenLen) {
  if (!Cxt) {
    return;
  }
  auto *WasiMod =
      dynamic_cast<WasmEdge::Host::WasiModule *>(fromImpObjCxt(Cxt));
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

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ImportObjectWASIGetExitCode(WasmEdge_ImportObjectContext *Cxt) {
  if (!Cxt) {
    return EXIT_FAILURE;
  }
  auto *WasiMod =
      dynamic_cast<WasmEdge::Host::WasiModule *>(fromImpObjCxt(Cxt));
  if (!WasiMod) {
    return EXIT_FAILURE;
  }
  return WasiMod->getEnv().getExitCode();
}

WASMEDGE_CAPI_EXPORT WasmEdge_ImportObjectContext *
WasmEdge_ImportObjectCreateWasmEdgeProcess(const char *const *AllowedCmds,
                                           const uint32_t CmdsLen,
                                           const bool AllowAll) {
  auto *ProcMod = new WasmEdge::Host::WasmEdgeProcessModule();
  WasmEdge_ImportObjectInitWasmEdgeProcess(toImpObjCxt(ProcMod), AllowedCmds,
                                           CmdsLen, AllowAll);
  return toImpObjCxt(ProcMod);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ImportObjectInitWasmEdgeProcess(
    WasmEdge_ImportObjectContext *Cxt, const char *const *AllowedCmds,
    const uint32_t CmdsLen, const bool AllowAll) {
  if (!Cxt) {
    return;
  }
  auto *ProcMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessModule *>(fromImpObjCxt(Cxt));
  if (!ProcMod) {
    return;
  }
  auto &ProcEnv = ProcMod->getEnv();
  ProcEnv.AllowedAll = AllowAll;
  if (AllowAll) {
    ProcEnv.AllowedCmd.clear();
  } else {
    for (uint32_t I = 0; I < CmdsLen; I++) {
      ProcEnv.AllowedCmd.insert(AllowedCmds[I]);
    }
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ImportObjectAddFunction(WasmEdge_ImportObjectContext *Cxt,
                                 const WasmEdge_String Name,
                                 WasmEdge_FunctionInstanceContext *FuncCxt) {
  if (Cxt && FuncCxt) {
    fromImpObjCxt(Cxt)->addHostFunc(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::FunctionInstance>(
            fromFuncCxt(FuncCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ImportObjectAddTable(WasmEdge_ImportObjectContext *Cxt,
                              const WasmEdge_String Name,
                              WasmEdge_TableInstanceContext *TableCxt) {
  if (Cxt && TableCxt) {
    fromImpObjCxt(Cxt)->addHostTable(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::TableInstance>(
            fromTabCxt(TableCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ImportObjectAddMemory(WasmEdge_ImportObjectContext *Cxt,
                               const WasmEdge_String Name,
                               WasmEdge_MemoryInstanceContext *MemoryCxt) {
  if (Cxt && MemoryCxt) {
    fromImpObjCxt(Cxt)->addHostMemory(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::MemoryInstance>(
            fromMemCxt(MemoryCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ImportObjectAddGlobal(WasmEdge_ImportObjectContext *Cxt,
                               const WasmEdge_String Name,
                               WasmEdge_GlobalInstanceContext *GlobalCxt) {
  if (Cxt && GlobalCxt) {
    fromImpObjCxt(Cxt)->addHostGlobal(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::Instance::GlobalInstance>(
            fromGlobCxt(GlobalCxt)));
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ImportObjectDelete(WasmEdge_ImportObjectContext *Cxt) {
  delete fromImpObjCxt(Cxt);
}

/// >>>>>>>> WasmEdge import object functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// >>>>>>>> WasmEdge Async functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncWait(WasmEdge_Async *Cxt) {
  Cxt->Async.wait();
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_AsyncWaitFor(WasmEdge_Async *Cxt,
                                                uint64_t Milliseconds) {
  return Cxt->Async.waitFor(std::chrono::milliseconds(Milliseconds));
}

WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncCancel(WasmEdge_Async *Cxt) {
  Cxt->Async.cancel();
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_AsyncGet(
    WasmEdge_Async *Cxt, WasmEdge_Value *Returns, const uint32_t ReturnLen) {
  return wrap(
      [&]() { return Cxt->Async.get(); },
      [&](auto Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncDelete(WasmEdge_Async *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge Async functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge VM functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      genSpan(Buf, BufLen));
      },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromImport(
    WasmEdge_VMContext *Cxt, const WasmEdge_ImportObjectContext *ImportCxt) {
  return wrap(
      [&]() { return Cxt->VM.registerModule(*fromImpObjCxt(ImportCxt)); },
      EmptyThen, Cxt, ImportCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMRegisterModuleFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      *ASTCxt->Module.get());
      },
      EmptyThen, Cxt, ASTCxt);
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
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(genSpan(Buf, BufLen), genStrView(FuncName),
                                   ParamPair.first, ParamPair.second);
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
        return Cxt->VM.runWasmFile(*ASTCxt->Module.get(), genStrView(FuncName),
                                   ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromFile(
    WasmEdge_VMContext *Cxt, const char *Path, const WasmEdge_String FuncName,
    const WasmEdge_Value *Params, const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return new WasmEdge_Async(Cxt->VM.asyncRunWasmFile(
      std::filesystem::absolute(Path), genStrView(FuncName), ParamPair.first,
      ParamPair.second));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return new WasmEdge_Async(
      Cxt->VM.asyncRunWasmFile(genSpan(Buf, BufLen), genStrView(FuncName),
                               ParamPair.first, ParamPair.second));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return new WasmEdge_Async(
      Cxt->VM.asyncRunWasmFile(*ASTCxt->Module.get(), genStrView(FuncName),
                               ParamPair.first, ParamPair.second));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_VMLoadWasmFromFile(WasmEdge_VMContext *Cxt, const char *Path) {
  return wrap(
      [&]() { return Cxt->VM.loadWasm(std::filesystem::absolute(Path)); },
      EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMLoadWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen) {
  return wrap([&]() { return Cxt->VM.loadWasm(genSpan(Buf, BufLen)); },
              EmptyThen, Cxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_VMLoadWasmFromASTModule(
    WasmEdge_VMContext *Cxt, const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap([&]() { return Cxt->VM.loadWasm(*ASTCxt->Module.get()); },
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
  return new WasmEdge_Async(Cxt->VM.asyncExecute(
      genStrView(FuncName), ParamPair.first, ParamPair.second));
}

WASMEDGE_CAPI_EXPORT WasmEdge_Async *WasmEdge_VMAsyncExecuteRegistered(
    WasmEdge_VMContext *Cxt, const WasmEdge_String ModuleName,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return new WasmEdge_Async(
      Cxt->VM.asyncExecute(genStrView(ModuleName), genStrView(FuncName),
                           ParamPair.first, ParamPair.second));
}

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_VMGetFunctionType(WasmEdge_VMContext *Cxt,
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
WasmEdge_VMGetFunctionTypeRegistered(WasmEdge_VMContext *Cxt,
                                     const WasmEdge_String ModuleName,
                                     const WasmEdge_String FuncName) {
  if (Cxt) {
    auto &Store = Cxt->VM.getStoreManager();
    if (auto Res = Store.findModule(genStrView(ModuleName))) {
      const auto *ModInst = *Res;
      const auto &FuncExp = ModInst->getFuncExports();
      const auto FuncIter = FuncExp.find(genStrView(FuncName));
      if (FuncIter != FuncExp.cend()) {
        const auto *FuncInst = *Store.getFunction(FuncIter->second);
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
WasmEdge_VMGetFunctionListLength(WasmEdge_VMContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(Cxt->VM.getFunctionList().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_VMGetFunctionList(
    WasmEdge_VMContext *Cxt, WasmEdge_String *Names,
    const WasmEdge_FunctionTypeContext **FuncTypes, const uint32_t Len) {
  if (Cxt) {
    /// Not to use VM::getFunctionList() here because not to allocate the
    /// returned function name strings.
    auto &Store = Cxt->VM.getStoreManager();
    if (auto Res = Store.getActiveModule()) {
      const auto &FuncExp = (*Res)->getFuncExports();
      uint32_t I = 0;
      for (auto It = FuncExp.cbegin(); It != FuncExp.cend() && I < Len;
           It++, I++) {
        const auto *FuncInst = *Store.getFunction(It->second);
        const auto &FuncType = FuncInst->getFuncType();
        if (Names) {
          Names[I] = WasmEdge_String{
              .Length = static_cast<uint32_t>(It->first.length()),
              .Buf = It->first.data()};
        }
        if (FuncTypes) {
          FuncTypes[I] = toFuncTypeCxt(&FuncType);
        }
      }
      return static_cast<uint32_t>(FuncExp.size());
    }
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT WasmEdge_ImportObjectContext *
WasmEdge_VMGetImportModuleContext(WasmEdge_VMContext *Cxt,
                                  const enum WasmEdge_HostRegistration Reg) {
  if (Cxt) {
    return toImpObjCxt(
        Cxt->VM.getImportModule(static_cast<WasmEdge::HostRegistration>(Reg)));
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

/// <<<<<<<< WasmEdge VM functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif
