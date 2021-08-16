// SPDX-License-Identifier: Apache-2.0
#include <cstring>
#include <vector>

#include "api/wasmedge.h"

#include "common/errcode.h"
#include "common/log.h"
#include "common/span.h"
#include "common/statistics.h"
#include "common/value.h"

#include "aot/compiler.h"
#include "ast/module.h"
#include "host/wasi/wasimodule.h"
#include "host/wasmedge_process/processmodule.h"
#include "interpreter/interpreter.h"
#include "loader/loader.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"
#include "vm/vm.h"

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

/// WasmEdge_InterpreterContext implementation.
struct WasmEdge_InterpreterContext {
  WasmEdge_InterpreterContext(
      const WasmEdge::Configure &Conf,
      WasmEdge::Statistics::Statistics *S = nullptr) noexcept
      : Interp(Conf, S) {}
  WasmEdge::Interpreter::Interpreter Interp;
};

/// WasmEdge_StoreContext implementation.
struct WasmEdge_StoreContext {};

/// WasmEdge_FunctionTypeContext implementation.
struct WasmEdge_FunctionTypeContext {};

/// WasmEdge_FunctionInstanceContext implementation.
struct WasmEdge_FunctionInstanceContext {};

/// WasmEdge_HostFunctionContext implementation.
struct WasmEdge_HostFunctionContext {};

/// WasmEdge_TableInstanceContext implementation.
struct WasmEdge_TableInstanceContext {};

/// WasmEdge_MemoryInstanceContext implementation.
struct WasmEdge_MemoryInstanceContext {};

/// WasmEdge_GlobalInstanceContext implementation.
struct WasmEdge_GlobalInstanceContext {};

/// WasmEdge_ImportObjectContext implementation.
struct WasmEdge_ImportObjectContext {};

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

/// Helper functions for returning a WasmEdge_Value by various values.
template <typename T> inline WasmEdge_Value genWasmEdge_Value(T Val) noexcept {
  return WasmEdge_Value{
      .Value = ValVariant(Val).unwrap(),
      .Type = static_cast<WasmEdge_ValType>(WasmEdge::ValTypeFromType<T>())};
}
template <>
inline constexpr WasmEdge_Value genWasmEdge_Value(__int128 Val) noexcept {
  return WasmEdge_Value{.Value = static_cast<unsigned __int128>(Val),
                        .Type = WasmEdge_ValType_V128};
}
inline WasmEdge_Value genWasmEdge_Value(ValVariant Val,
                                        WasmEdge_ValType T) noexcept {
  return WasmEdge_Value{.Value = Val.unwrap(), .Type = T};
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
      VVec[I] = ValVariant::wrap<uint32_t>(Val[I].Value);
      break;
    case ValType::I64:
      VVec[I] = ValVariant::wrap<uint64_t>(Val[I].Value);
      break;
    case ValType::F32:
      VVec[I] = ValVariant::wrap<float>(Val[I].Value);
      break;
    case ValType::F64:
      VVec[I] = ValVariant::wrap<double>(Val[I].Value);
      break;
    case ValType::V128:
      VVec[I] = ValVariant::wrap<unsigned __int128>(Val[I].Value);
      break;
    case ValType::FuncRef:
      VVec[I] = ValVariant::wrap<FuncRef>(Val[I].Value);
      break;
    case ValType::ExternRef:
      VVec[I] = ValVariant::wrap<ExternRef>(Val[I].Value);
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
inline constexpr void fillWasmEdge_ValueArr(Span<const ValVariant> Vec,
                                            WasmEdge_Value *Val,
                                            const uint32_t Len) noexcept {
  if (Val == nullptr) {
    return;
  }
  for (uint32_t I = 0; I < Len && I < Vec.size(); I++) {
    Val[I] = genWasmEdge_Value(Vec[I], WasmEdge_ValType_I32);
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
      uint32_t NameLen = static_cast<uint32_t>(Pair.first.length());
      char *Str = new char[NameLen];
      std::copy_n(&Pair.first.data()[0], NameLen, Str);
      Names[I] = WasmEdge_String{.Length = NameLen, .Buf = Str};
    }
    I++;
  }
  return static_cast<uint32_t>(Map.size());
}

/// C API Import module class
class CAPIImportModule : public Runtime::ImportObject {
public:
  CAPIImportModule(const WasmEdge_String Name, void *Ptr)
      : ImportObject(genStrView(Name)), Data(Ptr) {}

  void *getData() const { return Data; }

private:
  void *Data;
};

/// C API Host function class
class CAPIHostFunc : public Runtime::HostFunctionBase {
public:
  CAPIHostFunc(const Runtime::Instance::FType *Type,
               WasmEdge_HostFunc_t FuncPtr,
               const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(FuncPtr), Wrap(nullptr),
        Binding(nullptr), Data(nullptr) {
    FuncType = *Type;
  }
  CAPIHostFunc(const Runtime::Instance::FType *Type,
               WasmEdge_WrapFunc_t WrapPtr, void *BindingPtr,
               const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(nullptr), Wrap(WrapPtr),
        Binding(BindingPtr), Data(nullptr) {
    FuncType = *Type;
  }
  ~CAPIHostFunc() noexcept override = default;

  void setData(void *Ptr) { Data = Ptr; }

  Expect<void> run(Runtime::Instance::MemoryInstance *MemInst,
                   Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {

    std::vector<WasmEdge_Value> Params(FuncType.Params.size()),
        Returns(FuncType.Returns.size());
    for (uint32_t I = 0; I < Args.size(); I++) {
      Params[I].Value = Args[I].get<unsigned __int128>();
      Params[I].Type = static_cast<WasmEdge_ValType>(FuncType.Params[I]);
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
      Rets[I] = Returns[I].Value;
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
CONVTO(Store, Runtime::StoreManager, Store, )
CONVTO(FType, Runtime::Instance::FType, FunctionType, )
CONVTO(FType, Runtime::Instance::FType, FunctionType, const)
CONVTO(Func, Runtime::Instance::FunctionInstance, FunctionInstance, )
CONVTO(HostFunc, Runtime::HostFunctionBase, HostFunction, )
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
CONVFROM(Store, Runtime::StoreManager, Store, )
CONVFROM(Store, Runtime::StoreManager, Store, const)
CONVFROM(FType, Runtime::Instance::FType, FunctionType, const)
CONVFROM(Func, Runtime::Instance::FunctionInstance, FunctionInstance, const)
CONVFROM(HostFunc, CAPIHostFunc, HostFunction, )
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

WASMEDGE_CAPI_EXPORT WasmEdge_Value WasmEdge_ValueGenV128(const __int128 Val) {
  return genWasmEdge_Value(Val);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value
WasmEdge_ValueGenNullRef(const WasmEdge_RefType T) {
  return genWasmEdge_Value(
      WasmEdge::genNullRef(static_cast<WasmEdge::RefType>(T)),
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
  return WasmEdge::ValVariant::wrap<int32_t>(Val.Value).get<int32_t>();
}

WASMEDGE_CAPI_EXPORT int64_t WasmEdge_ValueGetI64(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<int64_t>(Val.Value).get<int64_t>();
}

WASMEDGE_CAPI_EXPORT float WasmEdge_ValueGetF32(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<float>(Val.Value).get<float>();
}

WASMEDGE_CAPI_EXPORT double WasmEdge_ValueGetF64(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<double>(Val.Value).get<double>();
}

WASMEDGE_CAPI_EXPORT __int128 WasmEdge_ValueGetV128(const WasmEdge_Value Val) {
  return WasmEdge::ValVariant::wrap<__int128>(Val.Value).get<__int128>();
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ValueIsNullRef(const WasmEdge_Value Val) {
  return WasmEdge::isNullRef(WasmEdge::ValVariant::wrap<UnknownRef>(Val.Value));
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_ValueGetFuncIdx(const WasmEdge_Value Val) {
  return WasmEdge::retrieveFuncIdx(
      WasmEdge::ValVariant::wrap<FuncRef>(Val.Value));
}

WASMEDGE_CAPI_EXPORT void *
WasmEdge_ValueGetExternRef(const WasmEdge_Value Val) {
  return &WasmEdge::retrieveExternRef<uint32_t>(
      WasmEdge::ValVariant::wrap<ExternRef>(Val.Value));
}

/// <<<<<<<< WasmEdge value functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// <<<<<<<< WasmEdge string functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_StringCreateByCString(const char *Str) {
  return WasmEdge_StringCreateByBuffer(Str,
                                       static_cast<uint32_t>(std::strlen(Str)));
}

WASMEDGE_CAPI_EXPORT WasmEdge_String
WasmEdge_StringCreateByBuffer(const char *Buf, const uint32_t Len) {
  char *Str = new char[Len];
  std::copy_n(Buf, Len, Str);
  return WasmEdge_String{.Length = Len, .Buf = Str};
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
WasmEdge_ConfigureCompilerSetInstructionCounting(WasmEdge_ConfigureContext *Cxt,
                                                 const bool IsCount) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setInstructionCounting(IsCount);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureCompilerIsInstructionCounting(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getCompilerConfigure().isInstructionCounting();
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_ConfigureCompilerSetCostMeasuring(WasmEdge_ConfigureContext *Cxt,
                                           const bool IsMeasure) {
  if (Cxt) {
    Cxt->Conf.getCompilerConfigure().setCostMeasuring(IsMeasure);
  }
}

WASMEDGE_CAPI_EXPORT bool WasmEdge_ConfigureCompilerIsCostMeasuring(
    const WasmEdge_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getCompilerConfigure().isCostMeasuring();
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

WASMEDGE_CAPI_EXPORT void
WasmEdge_ASTModuleDelete(WasmEdge_ASTModuleContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge AST module functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

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

/// >>>>>>>> WasmEdge interpreter functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_InterpreterContext *
WasmEdge_InterpreterCreate(const WasmEdge_ConfigureContext *ConfCxt,
                           WasmEdge_StatisticsContext *StatCxt) {
  if (ConfCxt) {
    if (StatCxt) {
      return new WasmEdge_InterpreterContext(ConfCxt->Conf,
                                             fromStatCxt(StatCxt));
    } else {
      return new WasmEdge_InterpreterContext(ConfCxt->Conf);
    }
  } else {
    if (StatCxt) {
      return new WasmEdge_InterpreterContext(WasmEdge::Configure(),
                                             fromStatCxt(StatCxt));
    } else {
      return new WasmEdge_InterpreterContext(WasmEdge::Configure());
    }
  }
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_InterpreterInstantiate(
    WasmEdge_InterpreterContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->Interp.instantiateModule(*fromStoreCxt(StoreCxt),
                                             *ASTCxt->Module.get());
      },
      EmptyThen, Cxt, StoreCxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_InterpreterRegisterImport(
    WasmEdge_InterpreterContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ImportObjectContext *ImportCxt) {
  return wrap(
      [&]() {
        return Cxt->Interp.registerModule(*fromStoreCxt(StoreCxt),
                                          *fromImpObjCxt(ImportCxt));
      },
      EmptyThen, Cxt, StoreCxt, ImportCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_InterpreterRegisterModule(
    WasmEdge_InterpreterContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ASTModuleContext *ASTCxt, const WasmEdge_String ModuleName) {
  return wrap(
      [&]() {
        return Cxt->Interp.registerModule(*fromStoreCxt(StoreCxt),
                                          *ASTCxt->Module.get(),
                                          genStrView(ModuleName));
      },
      EmptyThen, Cxt, StoreCxt, ASTCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_InterpreterInvoke(
    WasmEdge_InterpreterContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns,
    const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() -> WasmEdge::Expect<std::vector<WasmEdge::ValVariant>> {
        /// Check exports for finding function address.
        const auto FuncExp = fromStoreCxt(StoreCxt)->getFuncExports();
        const auto FuncIter = FuncExp.find(genStrView(FuncName));
        if (FuncIter == FuncExp.cend()) {
          spdlog::error(WasmEdge::ErrCode::FuncNotFound);
          spdlog::error(
              WasmEdge::ErrInfo::InfoExecuting("", genStrView(FuncName)));
          return Unexpect(WasmEdge::ErrCode::FuncNotFound);
        }
        return Cxt->Interp.invoke(*fromStoreCxt(StoreCxt), FuncIter->second,
                                  ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      StoreCxt);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_InterpreterInvokeRegistered(
    WasmEdge_InterpreterContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_String ModuleName, const WasmEdge_String FuncName,
    const WasmEdge_Value *Params, const uint32_t ParamLen,
    WasmEdge_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  auto ModStr = genStrView(ModuleName);
  auto FuncStr = genStrView(FuncName);
  return wrap(
      [&]() -> WasmEdge::Expect<std::vector<WasmEdge::ValVariant>> {
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
        const auto FuncExp = ModInst->getFuncExports();
        const auto FuncIter = FuncExp.find(FuncStr);
        if (FuncIter == FuncExp.cend()) {
          spdlog::error(WasmEdge::ErrCode::FuncNotFound);
          spdlog::error(WasmEdge::ErrInfo::InfoExecuting(ModStr, FuncStr));
          return Unexpect(WasmEdge::ErrCode::FuncNotFound);
        }
        return Cxt->Interp.invoke(*fromStoreCxt(StoreCxt), FuncIter->second,
                                  ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillWasmEdge_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      StoreCxt);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_InterpreterDelete(WasmEdge_InterpreterContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge interpreter functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_StoreContext *WasmEdge_StoreCreate(void) {
  return toStoreCxt(new WasmEdge::Runtime::StoreManager);
}

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionInstanceContext *
WasmEdge_StoreFindFunction(WasmEdge_StoreContext *Cxt,
                           const WasmEdge_String Name) {
  if (Cxt) {
    const auto FuncExp = fromStoreCxt(Cxt)->getFuncExports();
    const auto FuncIter = FuncExp.find(genStrView(Name));
    if (FuncIter != FuncExp.cend()) {
      return toFuncCxt(*fromStoreCxt(Cxt)->getFunction(FuncIter->second));
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
    const auto TabExp = fromStoreCxt(Cxt)->getTableExports();
    const auto TabIter = TabExp.find(genStrView(Name));
    if (TabIter != TabExp.cend()) {
      return toTabCxt(*fromStoreCxt(Cxt)->getTable(TabIter->second));
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
    const auto MemExp = fromStoreCxt(Cxt)->getMemExports();
    const auto MemIter = MemExp.find(genStrView(Name));
    if (MemIter != MemExp.cend()) {
      return toMemCxt(*fromStoreCxt(Cxt)->getMemory(MemIter->second));
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
    const auto GlobExp = fromStoreCxt(Cxt)->getGlobalExports();
    const auto GlobIter = GlobExp.find(genStrView(Name));
    if (GlobIter != GlobExp.cend()) {
      return toGlobCxt(*fromStoreCxt(Cxt)->getGlobal(GlobIter->second));
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
    return static_cast<uint32_t>(fromStoreCxt(Cxt)->getFuncExports().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListFunction(
    WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getFuncExports(), Names, Len);
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
    WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
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
    return static_cast<uint32_t>(fromStoreCxt(Cxt)->getTableExports().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListTable(
    WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getTableExports(), Names, Len);
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
    WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
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
    return static_cast<uint32_t>(fromStoreCxt(Cxt)->getMemExports().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListMemory(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getMemExports(), Names, Len);
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
    WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
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
    return static_cast<uint32_t>(fromStoreCxt(Cxt)->getGlobalExports().size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_StoreListGlobal(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getGlobalExports(), Names, Len);
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
    WasmEdge_StoreContext *Cxt, const WasmEdge_String ModuleName,
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

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_StoreListModule(
    WasmEdge_StoreContext *Cxt, WasmEdge_String *Names, const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getModuleList(), Names, Len);
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void WasmEdge_StoreDelete(WasmEdge_StoreContext *Cxt) {
  delete fromStoreCxt(Cxt);
}

/// <<<<<<<< WasmEdge store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge function type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionTypeContext *WasmEdge_FunctionTypeCreate(
    const enum WasmEdge_ValType *ParamList, const uint32_t ParamLen,
    const enum WasmEdge_ValType *ReturnList, const uint32_t ReturnLen) {
  auto *Cxt = new WasmEdge::Runtime::Instance::FType;
  if (ParamLen > 0) {
    Cxt->Params.resize(ParamLen);
  }
  for (uint32_t I = 0; I < ParamLen; I++) {
    Cxt->Params[I] = static_cast<WasmEdge::ValType>(ParamList[I]);
  }
  if (ReturnLen > 0) {
    Cxt->Returns.resize(ReturnLen);
  }
  for (uint32_t I = 0; I < ReturnLen; I++) {
    Cxt->Returns[I] = static_cast<WasmEdge::ValType>(ReturnList[I]);
  }
  return toFTypeCxt(Cxt);
}

WASMEDGE_CAPI_EXPORT uint32_t WasmEdge_FunctionTypeGetParametersLength(
    const WasmEdge_FunctionTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(fromFTypeCxt(Cxt)->Params.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_FunctionTypeGetParameters(const WasmEdge_FunctionTypeContext *Cxt,
                                   WasmEdge_ValType *List, const uint32_t Len) {
  if (Cxt) {
    for (uint32_t I = 0; I < fromFTypeCxt(Cxt)->Params.size() && I < Len; I++) {
      List[I] = static_cast<WasmEdge_ValType>(fromFTypeCxt(Cxt)->Params[I]);
    }
    return static_cast<uint32_t>(fromFTypeCxt(Cxt)->Params.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_FunctionTypeGetReturnsLength(const WasmEdge_FunctionTypeContext *Cxt) {
  if (Cxt) {
    return static_cast<uint32_t>(fromFTypeCxt(Cxt)->Returns.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_FunctionTypeGetReturns(const WasmEdge_FunctionTypeContext *Cxt,
                                WasmEdge_ValType *List, const uint32_t Len) {
  if (Cxt) {
    for (uint32_t I = 0; I < fromFTypeCxt(Cxt)->Returns.size() && I < Len;
         I++) {
      List[I] = static_cast<WasmEdge_ValType>(fromFTypeCxt(Cxt)->Returns[I]);
    }
    return static_cast<uint32_t>(fromFTypeCxt(Cxt)->Returns.size());
  }
  return 0;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_FunctionTypeDelete(WasmEdge_FunctionTypeContext *Cxt) {
  delete Cxt;
}

/// <<<<<<<< WasmEdge function type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge function instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT const WasmEdge_FunctionTypeContext *
WasmEdge_FunctionInstanceGetFunctionType(
    const WasmEdge_FunctionInstanceContext *Cxt) {
  if (Cxt) {
    return toFTypeCxt(&fromFuncCxt(Cxt)->getFuncType());
  }
  return nullptr;
}

/// <<<<<<<< WasmEdge function instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge host function functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_HostFunctionContext *
WasmEdge_HostFunctionCreate(const WasmEdge_FunctionTypeContext *Type,
                            WasmEdge_HostFunc_t HostFunc, const uint64_t Cost) {
  if (Type && HostFunc) {
    return toHostFuncCxt(new CAPIHostFunc(fromFTypeCxt(Type), HostFunc, Cost));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_HostFunctionContext *
WasmEdge_HostFunctionCreateBinding(const WasmEdge_FunctionTypeContext *Type,
                                   WasmEdge_WrapFunc_t WrapFunc, void *Binding,
                                   const uint64_t Cost) {
  if (Type && WrapFunc) {
    return toHostFuncCxt(
        new CAPIHostFunc(fromFTypeCxt(Type), WrapFunc, Binding, Cost));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_HostFunctionDelete(WasmEdge_HostFunctionContext *Cxt) {
  delete fromHostFuncCxt(Cxt);
}

/// <<<<<<<< WasmEdge host function functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> WasmEdge table instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASMEDGE_CAPI_EXPORT WasmEdge_TableInstanceContext *
WasmEdge_TableInstanceCreate(const enum WasmEdge_RefType RefType,
                             const WasmEdge_Limit Limit) {
  WasmEdge::RefType Type = static_cast<WasmEdge::RefType>(RefType);
  if (Limit.HasMax) {
    return toTabCxt(new WasmEdge::Runtime::Instance::TableInstance(
        Type, WasmEdge::AST::Limit(Limit.Min, Limit.Max)));
  } else {
    return toTabCxt(new WasmEdge::Runtime::Instance::TableInstance(
        Type, WasmEdge::AST::Limit(Limit.Min)));
  }
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_RefType
WasmEdge_TableInstanceGetRefType(const WasmEdge_TableInstanceContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_RefType>(fromTabCxt(Cxt)->getReferenceType());
  }
  return WasmEdge_RefType_FuncRef;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_TableInstanceGetData(const WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value *Data, const uint32_t Offset) {
  return wrap([&]() { return fromTabCxt(Cxt)->getRefAddr(Offset); },
              [&](auto &&Res) {
                *Data =
                    genWasmEdge_Value(Res->template get<UnknownRef>(),
                                      static_cast<WasmEdge_ValType>(
                                          fromTabCxt(Cxt)->getReferenceType()));
              },
              Cxt, Data);
}

WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_TableInstanceSetData(WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value Data, const uint32_t Offset) {
  return wrap(
      [&]() -> WasmEdge::Expect<void> {
        WasmEdge::RefType expType = fromTabCxt(Cxt)->getReferenceType();
        if (expType != static_cast<WasmEdge::RefType>(Data.Type)) {
          spdlog::error(WasmEdge::ErrCode::RefTypeMismatch);
          spdlog::error(WasmEdge::ErrInfo::InfoMismatch(
              static_cast<WasmEdge::ValType>(expType),
              static_cast<WasmEdge::ValType>(Data.Type)));
          return Unexpect(WasmEdge::ErrCode::RefTypeMismatch);
        }
        return fromTabCxt(Cxt)->setRefAddr(
            Offset, WasmEdge::ValVariant(Data.Value).get<UnknownRef>());
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
WasmEdge_MemoryInstanceCreate(const WasmEdge_Limit Limit) {
  if (Limit.HasMax) {
    return toMemCxt(new WasmEdge::Runtime::Instance::MemoryInstance(
        WasmEdge::AST::Limit(Limit.Min, Limit.Max)));
  } else {
    return toMemCxt(new WasmEdge::Runtime::Instance::MemoryInstance(
        WasmEdge::AST::Limit(Limit.Min)));
  }
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
    return fromMemCxt(Cxt)->getDataPageSize();
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
WasmEdge_GlobalInstanceCreate(const WasmEdge_Value Value,
                              const enum WasmEdge_Mutability Mut) {
  return toGlobCxt(new WasmEdge::Runtime::Instance::GlobalInstance(
      static_cast<WasmEdge::ValType>(Value.Type),
      static_cast<WasmEdge::ValMut>(Mut), Value.Value));
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_ValType
WasmEdge_GlobalInstanceGetValType(const WasmEdge_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_ValType>(fromGlobCxt(Cxt)->getValType());
  }
  return WasmEdge_ValType_I32;
}

WASMEDGE_CAPI_EXPORT enum WasmEdge_Mutability
WasmEdge_GlobalInstanceGetMutability(
    const WasmEdge_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return static_cast<WasmEdge_Mutability>(fromGlobCxt(Cxt)->getValMut());
  }
  return WasmEdge_Mutability_Const;
}

WASMEDGE_CAPI_EXPORT WasmEdge_Value
WasmEdge_GlobalInstanceGetValue(const WasmEdge_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return genWasmEdge_Value(
        fromGlobCxt(Cxt)->getValue(),
        static_cast<WasmEdge_ValType>(fromGlobCxt(Cxt)->getValType()));
  }
  return genWasmEdge_Value(
      WasmEdge::ValVariant(static_cast<unsigned __int128>(0)),
      WasmEdge_ValType_I32);
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_GlobalInstanceSetValue(WasmEdge_GlobalInstanceContext *Cxt,
                                const WasmEdge_Value Value) {
  if (Cxt && fromGlobCxt(Cxt)->getValMut() == WasmEdge::ValMut::Var &&
      static_cast<WasmEdge::ValType>(Value.Type) ==
          fromGlobCxt(Cxt)->getValType()) {
    fromGlobCxt(Cxt)->getValue() = Value.Value;
  }
}

WASMEDGE_CAPI_EXPORT void
WasmEdge_GlobalInstanceDelete(WasmEdge_GlobalInstanceContext *Cxt) {
  delete fromGlobCxt(Cxt);
}

/// <<<<<<<< WasmEdge global instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// <<<<<<<< WasmEdge import object functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

WASMEDGE_CAPI_EXPORT WasmEdge_ImportObjectContext *
WasmEdge_ImportObjectCreate(const WasmEdge_String ModuleName, void *Data) {
  return toImpObjCxt(new CAPIImportModule(ModuleName, Data));
}

WASMEDGE_CAPI_EXPORT WasmEdge_ImportObjectContext *
WasmEdge_ImportObjectCreateWASI(const char *const *Args, const uint32_t ArgLen,
                                const char *const *Envs, const uint32_t EnvLen,
                                const char *const *Dirs, const uint32_t DirLen,
                                const char *const *Preopens,
                                const uint32_t PreopenLen) {
  auto *WasiMod = new WasmEdge::Host::WasiModule();
  WasmEdge_ImportObjectInitWASI(toImpObjCxt(WasiMod), Args, ArgLen, Envs,
                                EnvLen, Dirs, DirLen, Preopens, PreopenLen);
  return toImpObjCxt(WasiMod);
}

WASMEDGE_CAPI_EXPORT void WasmEdge_ImportObjectInitWASI(
    WasmEdge_ImportObjectContext *Cxt, const char *const *Args,
    const uint32_t ArgLen, const char *const *Envs, const uint32_t EnvLen,
    const char *const *Dirs, const uint32_t DirLen, const char *const *Preopens,
    const uint32_t PreopenLen) {
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
  if (Dirs) {
    for (uint32_t I = 0; I < DirLen; I++) {
      DirVec.emplace_back(Dirs[I]);
    }
  }
  if (Preopens) {
    for (uint32_t I = 0; I < PreopenLen; I++) {
      DirVec.emplace_back(std::string(Preopens[I]) + ":" +
                          std::string(Preopens[I]));
    }
  }
  auto &WasiEnv = WasiMod->getEnv();
  WasiEnv.init(DirVec, ProgName, ArgVec, EnvVec);
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

WASMEDGE_CAPI_EXPORT void WasmEdge_ImportObjectAddHostFunction(
    WasmEdge_ImportObjectContext *Cxt, const WasmEdge_String Name,
    WasmEdge_HostFunctionContext *HostFuncCxt) {
  if (Cxt && HostFuncCxt) {
    auto *ImpMod = reinterpret_cast<CAPIImportModule *>(Cxt);
    auto *HostFunc = reinterpret_cast<CAPIHostFunc *>(HostFuncCxt);
    HostFunc->setData(ImpMod->getData());
    fromImpObjCxt(Cxt)->addHostFunc(
        genStrView(Name),
        std::unique_ptr<WasmEdge::Runtime::HostFunctionBase>(HostFunc));
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

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionTypeContext *
WasmEdge_VMGetFunctionType(WasmEdge_VMContext *Cxt,
                           const WasmEdge_String FuncName) {
  if (Cxt) {
    const auto FuncList = Cxt->VM.getFunctionList();
    for (const auto &It : FuncList) {
      if (It.first == genStrView(FuncName)) {
        return toFTypeCxt(new WasmEdge::Runtime::Instance::FType(It.second));
      }
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT WasmEdge_FunctionTypeContext *
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
        return toFTypeCxt(
            new WasmEdge::Runtime::Instance::FType(FuncInst->getFuncType()));
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
    WasmEdge_FunctionTypeContext **FuncTypes, const uint32_t Len) {
  if (Cxt) {
    auto FuncList = Cxt->VM.getFunctionList();
    for (uint32_t I = 0; I < Len && I < FuncList.size(); I++) {
      if (Names) {
        uint32_t NameLen = static_cast<uint32_t>(FuncList[I].first.length());
        char *Str = new char[NameLen];
        std::copy_n(&FuncList[I].first.data()[0], NameLen, Str);
        Names[I] = WasmEdge_String{.Length = NameLen, .Buf = Str};
      }
      if (FuncTypes) {
        FuncTypes[I] = toFTypeCxt(
            new WasmEdge::Runtime::Instance::FType(FuncList[I].second));
      }
    }
    return static_cast<uint32_t>(FuncList.size());
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
