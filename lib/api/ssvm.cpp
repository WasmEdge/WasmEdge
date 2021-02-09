// SPDX-License-Identifier: Apache-2.0
#include <cstring>
#include <vector>

#include "api/ssvm.h"

#include "common/errcode.h"
#include "common/log.h"
#include "common/span.h"
#include "common/statistics.h"
#include "common/value.h"

#include "ast/module.h"
#include "host/wasi/wasimodule.h"
#include "interpreter/interpreter.h"
#include "loader/loader.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"
#include "vm/vm.h"

/// SSVM_ConfigureContext implementation.
struct SSVM_ConfigureContext {
  SSVM::Configure Conf;
};

/// SSVM_StatisticsContext implementation.
struct SSVM_StatisticsContext {};

/// SSVM_ASTModuleContext implementation.
struct SSVM_ASTModuleContext {
  SSVM_ASTModuleContext(std::unique_ptr<SSVM::AST::Module> Mod) noexcept
      : Module(std::move(Mod)) {}
  std::unique_ptr<SSVM::AST::Module> Module;
};

/// SSVM_LoaderContext implementation.
struct SSVM_LoaderContext {
  SSVM_LoaderContext(const SSVM::Configure &Conf) noexcept : Load(Conf) {}
  SSVM::Loader::Loader Load;
};

/// SSVM_ValidatorContext implementation.
struct SSVM_ValidatorContext {
  SSVM_ValidatorContext(const SSVM::Configure &Conf) noexcept : Valid(Conf) {}
  SSVM::Validator::Validator Valid;
};

/// SSVM_InterpreterContext implementation.
struct SSVM_InterpreterContext {
  SSVM_InterpreterContext(const SSVM::Configure &Conf,
                          SSVM::Statistics::Statistics *S = nullptr) noexcept
      : Interp(Conf, S) {}
  SSVM::Interpreter::Interpreter Interp;
};

/// SSVM_StoreContext implementation.
struct SSVM_StoreContext {};

/// SSVM_FunctionTypeContext implementation.
struct SSVM_FunctionTypeContext {};

/// SSVM_FunctionInstanceContext implementation.
struct SSVM_FunctionInstanceContext {};

/// SSVM_HostFunctionContext implementation.
struct SSVM_HostFunctionContext {};

/// SSVM_TableInstanceContext implementation.
struct SSVM_TableInstanceContext {};

/// SSVM_MemoryInstanceContext implementation.
struct SSVM_MemoryInstanceContext {};

/// SSVM_GlobalInstanceContext implementation.
struct SSVM_GlobalInstanceContext {};

/// SSVM_ImportObjectContext implementation.
struct SSVM_ImportObjectContext {};

/// SSVM_VMContext implementation.
struct SSVM_VMContext {
  template <typename... Args>
  SSVM_VMContext(Args &&... Vals) noexcept : VM(std::forward<Args>(Vals)...) {}
  SSVM::VM::VM VM;
};

namespace {

using namespace SSVM;

/// Helper function for returning a SSVM_Result by error code.
inline constexpr SSVM_Result genSSVM_Result(ErrCode Code) noexcept {
  return SSVM_Result{.Code = static_cast<uint8_t>(Code)};
}

/// Helper functions for returning a SSVM_Value by various values.
template <typename T> inline SSVM_Value genSSVM_Value(T Val) noexcept {
  return SSVM_Value{
      .Value = retrieveValue<unsigned __int128>(ValVariant(toUnsigned(Val))),
      .Type = static_cast<SSVM_ValType>(SSVM::ValTypeFromType<T>())};
}
template <> inline constexpr SSVM_Value genSSVM_Value(__int128 Val) noexcept {
  return SSVM_Value{.Value = static_cast<unsigned __int128>(Val),
                    .Type = SSVM_ValType_V128};
}
inline SSVM_Value genSSVM_Value(ValVariant Val, SSVM_ValType T) noexcept {
  return SSVM_Value{.Value = retrieveValue<unsigned __int128>(Val), .Type = T};
}
inline SSVM_Value genSSVM_Value(RefVariant Val, SSVM_ValType T) noexcept {
  return genSSVM_Value(ValVariant(Val), T);
}

/// Helper function for converting a SSVM_Value array to a ValVariant vector.
inline std::pair<std::vector<ValVariant>, std::vector<ValType>>
genParamPair(const SSVM_Value *Val, const uint32_t Len) noexcept {
  std::vector<ValVariant> VVec;
  std::vector<ValType> TVec;
  if (Val == nullptr) {
    return {VVec, TVec};
  }
  VVec.resize(Len);
  TVec.resize(Len);
  for (uint32_t I = 0; I < Len; I++) {
    VVec[I] = Val[I].Value;
    TVec[I] = static_cast<ValType>(Val[I].Type);
  }
  return {VVec, TVec};
}

/// Helper function for making a Span to a uint8_t array.
template <typename T>
inline constexpr Span<const T> genSpan(const T *Buf,
                                       const uint32_t Len) noexcept {
  return Span<const T>(Buf, Len);
}

/// Helper functions for converting SSVM_String to std::String.
inline std::string_view genStrView(const SSVM_String S) noexcept {
  return std::string_view(S.Buf, S.Length);
}

/// Helper functions for converting a ValVariant vector to a SSVM_Value array.
inline constexpr void fillSSVM_ValueArr(Span<const ValVariant> Vec,
                                        SSVM_Value *Val,
                                        const uint32_t Len) noexcept {
  if (Val == nullptr) {
    return;
  }
  for (uint32_t I = 0; I < Len && I < Vec.size(); I++) {
    Val[I] = genSSVM_Value(Vec[I], SSVM_ValType_I32);
  }
}

/// Helper template to run and return result.
auto EmptyThen = [](auto &&Res) noexcept {};
template <typename T> inline bool isContext(T *Cxt) noexcept {
  return (Cxt != nullptr);
}
template <typename T, typename... Args>
inline bool isContext(T *Cxt, Args *... Cxts) noexcept {
  return isContext(Cxt) && isContext(Cxts...);
}
template <typename T, typename U, typename... CxtT>
inline SSVM_Result wrap(T &&Proc, U &&Then, CxtT *... Cxts) noexcept {
  if (isContext(Cxts...)) {
    if (auto Res = Proc()) {
      Then(Res);
      return genSSVM_Result(ErrCode::Success);
    } else {
      return genSSVM_Result(Res.error());
    }
  } else {
    return genSSVM_Result(ErrCode::WrongVMWorkflow);
  }
}

/// Helper template function for deletion.
template <typename T> inline constexpr void deleteIf(T *Cxt) noexcept {
  if (Cxt) {
    delete Cxt;
  }
}

/// Helper function of retrieving exported maps.
inline uint32_t fillMap(const std::map<std::string, uint32_t, std::less<>> &Map,
                        SSVM_String *Names, const uint32_t Len) noexcept {
  uint32_t I = 0;
  for (auto &&Pair : Map) {
    if (I >= Len) {
      break;
    }
    if (Names) {
      uint32_t NameLen = Pair.first.length();
      char *Str = new char[NameLen];
      std::copy_n(&Pair.first.data()[0], NameLen, Str);
      Names[I] = SSVM_String{.Length = NameLen, .Buf = Str};
    }
    I++;
  }
  return Map.size();
}

/// C API Import module class
class CAPIImportModule : public Runtime::ImportObject {
public:
  CAPIImportModule(const SSVM_String ModName, void *Ptr)
      : ImportObject(genStrView(ModName)), Data(Ptr) {}

  void *getData() const { return Data; }

private:
  void *Data;
};

/// C API Host function class
class CAPIHostFunc : public Runtime::HostFunctionBase {
public:
  CAPIHostFunc(const Runtime::Instance::FType *Type, SSVM_HostFunc_t FuncPtr,
               const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(FuncPtr), Wrap(nullptr),
        Binding(nullptr), Data(nullptr) {
    FuncType = *Type;
  }
  CAPIHostFunc(const Runtime::Instance::FType *Type, SSVM_WrapFunc_t WrapPtr,
               void *BindingPtr, const uint64_t FuncCost = 0) noexcept
      : Runtime::HostFunctionBase(FuncCost), Func(nullptr), Wrap(WrapPtr),
        Binding(BindingPtr), Data(nullptr) {
    FuncType = *Type;
  }
  virtual ~CAPIHostFunc() noexcept = default;

  void setData(void *Ptr) { Data = Ptr; }

  Expect<void> run(Runtime::Instance::MemoryInstance *MemInst,
                   Span<const ValVariant> Args,
                   Span<ValVariant> Rets) override {

    std::vector<SSVM_Value> Params(FuncType.Params.size()),
        Returns(FuncType.Returns.size());
    for (uint32_t I = 0; I < Args.size(); I++) {
      Params[I].Value = retrieveValue<__int128>(Args[I]);
      Params[I].Type = static_cast<SSVM_ValType>(FuncType.Params[I]);
    }
    SSVM_Value *PPtr = Params.size() ? (&Params[0]) : nullptr;
    SSVM_Value *RPtr = Returns.size() ? (&Returns[0]) : nullptr;
    auto *MemCxt = reinterpret_cast<SSVM_MemoryInstanceContext *>(MemInst);
    SSVM_Result Stat;
    if (Func) {
      Stat = Func(Data, MemCxt, PPtr, RPtr);
    } else {
      Stat = Wrap(Binding, Data, MemCxt, PPtr, Params.size(), RPtr,
                  Returns.size());
    }
    for (uint32_t I = 0; I < Rets.size(); I++) {
      Rets[I] = Returns[I].Value;
    }
    if (!SSVM_ResultOK(Stat)) {
      return Unexpect(ErrCode::ExecutionFailed);
    } else if (Stat.Code == 0x01) {
      return Unexpect(ErrCode::Terminated);
    }
    return {};
  }

private:
  SSVM_HostFunc_t Func;
  SSVM_WrapFunc_t Wrap;
  void *Binding;
  void *Data;
};

/// Helper functions of context conversions.
#define CONVTO(SIMP, INST, NAME, QUANT)                                        \
  inline QUANT auto *to##SIMP##Cxt(QUANT INST *Cxt) noexcept {                 \
    return reinterpret_cast<QUANT SSVM_##NAME##Context *>(Cxt);                \
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
      QUANT SSVM_##NAME##Context *Cxt) noexcept {                              \
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

/// >>>>>>>> SSVM version functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

const char *SSVM_VersionGet() { return SSVM_VERSION; }

uint32_t SSVM_VersionGetMajor() { return SSVM_VERSION_MAJOR; }

uint32_t SSVM_VersionGetMinor() { return SSVM_VERSION_MINOR; }

uint32_t SSVM_VersionGetPatch() { return SSVM_VERSION_PATCH; }

/// <<<<<<<< SSVM version functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM logging functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void SSVM_LogSetErrorLevel() { SSVM::Log::setErrorLoggingLevel(); }

void SSVM_LogSetDebugLevel() { SSVM::Log::setDebugLoggingLevel(); }

/// <<<<<<<< SSVM logging functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM value functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_Value SSVM_ValueGenI32(const int32_t Val) { return genSSVM_Value(Val); }

SSVM_Value SSVM_ValueGenI64(const int64_t Val) { return genSSVM_Value(Val); }

SSVM_Value SSVM_ValueGenF32(const float Val) { return genSSVM_Value(Val); }

SSVM_Value SSVM_ValueGenF64(const double Val) { return genSSVM_Value(Val); }

SSVM_Value SSVM_ValueGenV128(const __int128 Val) { return genSSVM_Value(Val); }

SSVM_Value SSVM_ValueGenNullRef(const SSVM_RefType T) {
  return genSSVM_Value(SSVM::genNullRef(static_cast<SSVM::RefType>(T)),
                       static_cast<SSVM_ValType>(T));
}

SSVM_Value SSVM_ValueGenFuncRef(const uint32_t Index) {
  return genSSVM_Value(SSVM::genFuncRef(Index), SSVM_ValType_FuncRef);
}

SSVM_Value SSVM_ValueGenExternRef(void *Ref) {
  return genSSVM_Value(SSVM::genExternRef(Ref), SSVM_ValType_ExternRef);
}

int32_t SSVM_ValueGetI32(const SSVM_Value Val) {
  return SSVM::retrieveValue<int32_t>(SSVM::ValVariant(Val.Value));
}

int64_t SSVM_ValueGetI64(const SSVM_Value Val) {
  return SSVM::retrieveValue<int64_t>(SSVM::ValVariant(Val.Value));
}

float SSVM_ValueGetF32(const SSVM_Value Val) {
  return SSVM::retrieveValue<float>(SSVM::ValVariant(Val.Value));
}

double SSVM_ValueGetF64(const SSVM_Value Val) {
  return SSVM::retrieveValue<double>(SSVM::ValVariant(Val.Value));
}

__int128 SSVM_ValueGetV128(const SSVM_Value Val) {
  return SSVM::retrieveValue<__int128>(SSVM::ValVariant(Val.Value));
}

bool SSVM_ValueIsNullRef(const SSVM_Value Val) {
  return SSVM::isNullRef(SSVM::ValVariant(Val.Value));
}

uint32_t SSVM_ValueGetFuncIdx(const SSVM_Value Val) {
  return SSVM::retrieveFuncIdx(SSVM::ValVariant(Val.Value));
}

void *SSVM_ValueGetExternRef(const SSVM_Value Val) {
  return &SSVM::retrieveExternRef<uint32_t>(SSVM::ValVariant(Val.Value));
}

/// <<<<<<<< SSVM value functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// <<<<<<<< SSVM string functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

SSVM_String SSVM_StringCreateByCString(const char *Str) {
  return SSVM_StringCreateByBuffer(Str, std::strlen(Str));
}

SSVM_String SSVM_StringCreateByBuffer(const char *Buf, const uint32_t Len) {
  char *Str = new char[Len];
  std::copy_n(Buf, Len, Str);
  return SSVM_String{.Length = Len, .Buf = Str};
}

SSVM_String SSVM_StringWrap(const char *Buf, const uint32_t Len) {
  return SSVM_String{.Length = Len, .Buf = Buf};
}

bool SSVM_StringIsEqual(const SSVM_String Str1, const SSVM_String Str2) {
  if (Str1.Length != Str2.Length) {
    return false;
  }
  return std::equal(Str1.Buf, Str1.Buf + Str1.Length, Str2.Buf);
}

void SSVM_StringDelete(SSVM_String Str) {
  if (Str.Buf) {
    delete[] Str.Buf;
  }
}

/// >>>>>>>> SSVM string functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// >>>>>>>> SSVM result functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool SSVM_ResultOK(const SSVM_Result Res) {
  if (static_cast<SSVM::ErrCode>(Res.Code) == SSVM::ErrCode::Success ||
      static_cast<SSVM::ErrCode>(Res.Code) == SSVM::ErrCode::Terminated) {
    return true;
  } else {
    return false;
  }
}

uint32_t SSVM_ResultGetCode(const SSVM_Result Res) {
  return static_cast<uint32_t>(Res.Code);
}

const char *SSVM_ResultGetMessage(const SSVM_Result Res) {
  return SSVM::ErrCodeStr[static_cast<SSVM::ErrCode>(Res.Code)].c_str();
}

/// <<<<<<<< SSVM result functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM configure functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_ConfigureContext *SSVM_ConfigureCreate() {
  return new SSVM_ConfigureContext;
}

void SSVM_ConfigureAddProposal(SSVM_ConfigureContext *Cxt,
                               const SSVM_Proposal Prop) {
  if (Cxt) {
    Cxt->Conf.addProposal(static_cast<SSVM::Proposal>(Prop));
  }
}

void SSVM_ConfigureRemoveProposal(SSVM_ConfigureContext *Cxt,
                                  const SSVM_Proposal Prop) {
  if (Cxt) {
    Cxt->Conf.removeProposal(static_cast<SSVM::Proposal>(Prop));
  }
}

bool SSVM_ConfigureHasProposal(const SSVM_ConfigureContext *Cxt,
                               const SSVM_Proposal Prop) {
  if (Cxt) {
    return Cxt->Conf.hasProposal(static_cast<SSVM::Proposal>(Prop));
  }
  return false;
}

void SSVM_ConfigureAddHostRegistration(SSVM_ConfigureContext *Cxt,
                                       const SSVM_HostRegistration Host) {
  if (Cxt) {
    Cxt->Conf.addHostRegistration(static_cast<SSVM::HostRegistration>(Host));
  }
}

void SSVM_ConfigureRemoveHostRegistration(SSVM_ConfigureContext *Cxt,
                                          const SSVM_HostRegistration Host) {
  if (Cxt) {
    Cxt->Conf.removeHostRegistration(static_cast<SSVM::HostRegistration>(Host));
  }
}

bool SSVM_ConfigureHasHostRegistration(const SSVM_ConfigureContext *Cxt,
                                       const SSVM_HostRegistration Host) {
  if (Cxt) {
    return Cxt->Conf.hasHostRegistration(
        static_cast<SSVM::HostRegistration>(Host));
  }
  return false;
}

void SSVM_ConfigureSetMaxMemoryPage(SSVM_ConfigureContext *Cxt,
                                    const uint32_t Page) {
  if (Cxt) {
    Cxt->Conf.setMaxMemoryPage(Page);
  }
}

uint32_t SSVM_ConfigureGetMaxMemoryPage(const SSVM_ConfigureContext *Cxt) {
  if (Cxt) {
    return Cxt->Conf.getMaxMemoryPage();
  }
  return 0;
}

void SSVM_ConfigureDelete(SSVM_ConfigureContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM configure functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM statistics functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_StatisticsContext *SSVM_StatisticsCreate() {
  return toStatCxt(new SSVM::Statistics::Statistics);
}

uint64_t SSVM_StatisticsGetInstrCount(const SSVM_StatisticsContext *Cxt) {
  if (Cxt) {
    return fromStatCxt(Cxt)->getInstrCount();
  }
  return 0;
}

double SSVM_StatisticsGetInstrPerSecond(const SSVM_StatisticsContext *Cxt) {
  if (Cxt) {
    return fromStatCxt(Cxt)->getInstrPerSecond();
  }
  return 0.0;
}

uint64_t SSVM_StatisticsGetTotalCost(const SSVM_StatisticsContext *Cxt) {
  if (Cxt) {
    return fromStatCxt(Cxt)->getTotalCost();
  }
  return 0;
}

void SSVM_StatisticsSetCostTable(SSVM_StatisticsContext *Cxt, uint64_t *CostArr,
                                 const uint32_t Len) {
  if (Cxt) {
    fromStatCxt(Cxt)->setCostTable(genSpan(CostArr, Len));
  }
}

void SSVM_StatisticsSetCostLimit(SSVM_StatisticsContext *Cxt,
                                 const uint64_t Limit) {
  if (Cxt) {
    fromStatCxt(Cxt)->setCostLimit(Limit);
  }
}

void SSVM_StatisticsDelete(SSVM_StatisticsContext *Cxt) {
  deleteIf(fromStatCxt(Cxt));
}

/// <<<<<<<< SSVM statistics functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM AST module functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void SSVM_ASTModuleDelete(SSVM_ASTModuleContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM AST module functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM loader functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_LoaderContext *SSVM_LoaderCreate(const SSVM_ConfigureContext *ConfCxt) {
  if (ConfCxt) {
    return new SSVM_LoaderContext(ConfCxt->Conf);
  } else {
    return new SSVM_LoaderContext(SSVM::Configure());
  }
}

SSVM_Result SSVM_LoaderParseFromFile(SSVM_LoaderContext *Cxt,
                                     SSVM_ASTModuleContext **Module,
                                     const char *Path) {
  return wrap(
      [&]() { return Cxt->Load.parseModule(Path); },
      [&](auto &&Res) { *Module = new SSVM_ASTModuleContext(std::move(*Res)); },
      Cxt, Module);
}

SSVM_Result SSVM_LoaderParseFromBuffer(SSVM_LoaderContext *Cxt,
                                       SSVM_ASTModuleContext **Module,
                                       const uint8_t *Buf,
                                       const uint32_t BufLen) {
  return wrap(
      [&]() { return Cxt->Load.parseModule(genSpan(Buf, BufLen)); },
      [&](auto &&Res) { *Module = new SSVM_ASTModuleContext(std::move(*Res)); },
      Cxt, Module);
}

void SSVM_LoaderDelete(SSVM_LoaderContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM loader functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM validator functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_ValidatorContext *
SSVM_ValidatorCreate(const SSVM_ConfigureContext *ConfCxt) {
  if (ConfCxt) {
    return new SSVM_ValidatorContext(ConfCxt->Conf);
  } else {
    return new SSVM_ValidatorContext(SSVM::Configure());
  }
}

SSVM_Result SSVM_ValidatorValidate(SSVM_ValidatorContext *Cxt,
                                   const SSVM_ASTModuleContext *ModuleCxt) {
  return wrap([&]() { return Cxt->Valid.validate(*ModuleCxt->Module.get()); },
              EmptyThen, Cxt, ModuleCxt);
}

void SSVM_ValidatorDelete(SSVM_ValidatorContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM validator functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM interpreter functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_InterpreterContext *
SSVM_InterpreterCreate(const SSVM_ConfigureContext *ConfCxt,
                       SSVM_StatisticsContext *StatCxt) {
  if (ConfCxt) {
    if (StatCxt) {
      return new SSVM_InterpreterContext(ConfCxt->Conf, fromStatCxt(StatCxt));
    } else {
      return new SSVM_InterpreterContext(ConfCxt->Conf);
    }
  } else {
    if (StatCxt) {
      return new SSVM_InterpreterContext(SSVM::Configure(),
                                         fromStatCxt(StatCxt));
    } else {
      return new SSVM_InterpreterContext(SSVM::Configure());
    }
  }
}

SSVM_Result SSVM_InterpreterInstantiate(SSVM_InterpreterContext *Cxt,
                                        SSVM_StoreContext *StoreCxt,
                                        const SSVM_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->Interp.instantiateModule(*fromStoreCxt(StoreCxt),
                                             *ASTCxt->Module.get());
      },
      EmptyThen, Cxt, StoreCxt, ASTCxt);
}

SSVM_Result
SSVM_InterpreterRegisterImport(SSVM_InterpreterContext *Cxt,
                               SSVM_StoreContext *StoreCxt,
                               const SSVM_ImportObjectContext *ImportCxt) {
  return wrap(
      [&]() {
        return Cxt->Interp.registerModule(*fromStoreCxt(StoreCxt),
                                          *fromImpObjCxt(ImportCxt));
      },
      EmptyThen, Cxt, StoreCxt, ImportCxt);
}

SSVM_Result SSVM_InterpreterRegisterModule(SSVM_InterpreterContext *Cxt,
                                           SSVM_StoreContext *StoreCxt,
                                           const SSVM_ASTModuleContext *ASTCxt,
                                           const SSVM_String ModuleName) {
  return wrap(
      [&]() {
        return Cxt->Interp.registerModule(*fromStoreCxt(StoreCxt),
                                          *ASTCxt->Module.get(),
                                          genStrView(ModuleName));
      },
      EmptyThen, Cxt, StoreCxt, ASTCxt);
}

SSVM_Result SSVM_InterpreterInvoke(SSVM_InterpreterContext *Cxt,
                                   SSVM_StoreContext *StoreCxt,
                                   const SSVM_String FuncName,
                                   const SSVM_Value *Params,
                                   const uint32_t ParamLen, SSVM_Value *Returns,
                                   const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() -> SSVM::Expect<std::vector<SSVM::ValVariant>> {
        /// Check exports for finding function address.
        const auto FuncExp = fromStoreCxt(StoreCxt)->getFuncExports();
        const auto FuncIter = FuncExp.find(genStrView(FuncName));
        if (FuncIter == FuncExp.cend()) {
          LOG(ERROR) << SSVM::ErrCode::FuncNotFound;
          LOG(ERROR) << SSVM::ErrInfo::InfoExecuting("", genStrView(FuncName));
          return Unexpect(SSVM::ErrCode::FuncNotFound);
        }
        return Cxt->Interp.invoke(*fromStoreCxt(StoreCxt), FuncIter->second,
                                  ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      StoreCxt);
}

SSVM_Result SSVM_InterpreterInvokeRegistered(
    SSVM_InterpreterContext *Cxt, SSVM_StoreContext *StoreCxt,
    const SSVM_String ModuleName, const SSVM_String FuncName,
    const SSVM_Value *Params, const uint32_t ParamLen, SSVM_Value *Returns,
    const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  auto ModStr = genStrView(ModuleName);
  auto FuncStr = genStrView(FuncName);
  return wrap(
      [&]() -> SSVM::Expect<std::vector<SSVM::ValVariant>> {
        /// Get module instance.
        SSVM::Runtime::Instance::ModuleInstance *ModInst;
        if (auto Res = fromStoreCxt(StoreCxt)->findModule(ModStr)) {
          ModInst = *Res;
        } else {
          LOG(ERROR) << Res.error();
          LOG(ERROR) << SSVM::ErrInfo::InfoExecuting(ModStr, FuncStr);
          return Unexpect(Res);
        }

        /// Get exports and find function.
        const auto FuncExp = ModInst->getFuncExports();
        const auto FuncIter = FuncExp.find(FuncStr);
        if (FuncIter == FuncExp.cend()) {
          LOG(ERROR) << SSVM::ErrCode::FuncNotFound;
          LOG(ERROR) << SSVM::ErrInfo::InfoExecuting(ModStr, FuncStr);
          return Unexpect(SSVM::ErrCode::FuncNotFound);
        }
        return Cxt->Interp.invoke(*fromStoreCxt(StoreCxt), FuncIter->second,
                                  ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      StoreCxt);
}

void SSVM_InterpreterDelete(SSVM_InterpreterContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM interpreter functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_StoreContext *SSVM_StoreCreate() {
  return toStoreCxt(new SSVM::Runtime::StoreManager);
}

SSVM_FunctionInstanceContext *SSVM_StoreFindFunction(SSVM_StoreContext *Cxt,
                                                     const SSVM_String Name) {
  if (Cxt) {
    const auto FuncExp = fromStoreCxt(Cxt)->getFuncExports();
    const auto FuncIter = FuncExp.find(genStrView(Name));
    if (FuncIter != FuncExp.cend()) {
      return toFuncCxt(*fromStoreCxt(Cxt)->getFunction(FuncIter->second));
    }
  }
  return nullptr;
}

SSVM_FunctionInstanceContext *
SSVM_StoreFindFunctionRegistered(SSVM_StoreContext *Cxt,
                                 const SSVM_String ModuleName,
                                 const SSVM_String FuncName) {
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

SSVM_TableInstanceContext *SSVM_StoreFindTable(SSVM_StoreContext *Cxt,
                                               const SSVM_String Name) {
  if (Cxt) {
    const auto TabExp = fromStoreCxt(Cxt)->getTableExports();
    const auto TabIter = TabExp.find(genStrView(Name));
    if (TabIter != TabExp.cend()) {
      return toTabCxt(*fromStoreCxt(Cxt)->getTable(TabIter->second));
    }
  }
  return nullptr;
}

SSVM_TableInstanceContext *
SSVM_StoreFindTableRegistered(SSVM_StoreContext *Cxt,
                              const SSVM_String ModuleName,
                              const SSVM_String TableName) {
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

SSVM_MemoryInstanceContext *SSVM_StoreFindMemory(SSVM_StoreContext *Cxt,
                                                 const SSVM_String Name) {
  if (Cxt) {
    const auto MemExp = fromStoreCxt(Cxt)->getMemExports();
    const auto MemIter = MemExp.find(genStrView(Name));
    if (MemIter != MemExp.cend()) {
      return toMemCxt(*fromStoreCxt(Cxt)->getMemory(MemIter->second));
    }
  }
  return nullptr;
}

SSVM_MemoryInstanceContext *
SSVM_StoreFindMemoryRegistered(SSVM_StoreContext *Cxt,
                               const SSVM_String ModuleName,
                               const SSVM_String MemoryName) {
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

SSVM_GlobalInstanceContext *SSVM_StoreFindGlobal(SSVM_StoreContext *Cxt,
                                                 const SSVM_String Name) {
  if (Cxt) {
    const auto GlobExp = fromStoreCxt(Cxt)->getGlobalExports();
    const auto GlobIter = GlobExp.find(genStrView(Name));
    if (GlobIter != GlobExp.cend()) {
      return toGlobCxt(*fromStoreCxt(Cxt)->getGlobal(GlobIter->second));
    }
  }
  return nullptr;
}

SSVM_GlobalInstanceContext *
SSVM_StoreFindGlobalRegistered(SSVM_StoreContext *Cxt,
                               const SSVM_String ModuleName,
                               const SSVM_String GlobalName) {
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

uint32_t SSVM_StoreListFunctionLength(const SSVM_StoreContext *Cxt) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getFuncExports().size();
  }
  return 0;
}

uint32_t SSVM_StoreListFunction(SSVM_StoreContext *Cxt, SSVM_String *Names,
                                const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getFuncExports(), Names, Len);
  }
  return 0;
}

uint32_t SSVM_StoreListFunctionRegisteredLength(const SSVM_StoreContext *Cxt,
                                                const SSVM_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return (*Res)->getFuncExports().size();
    }
  }
  return 0;
}

uint32_t SSVM_StoreListFunctionRegistered(SSVM_StoreContext *Cxt,
                                          const SSVM_String ModuleName,
                                          SSVM_String *Names,
                                          const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getFuncExports(), Names, Len);
    }
  }
  return 0;
}

uint32_t SSVM_StoreListTableLength(const SSVM_StoreContext *Cxt) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getTableExports().size();
  }
  return 0;
}

uint32_t SSVM_StoreListTable(SSVM_StoreContext *Cxt, SSVM_String *Names,
                             const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getTableExports(), Names, Len);
  }
  return 0;
}

uint32_t SSVM_StoreListTableRegisteredLength(const SSVM_StoreContext *Cxt,
                                             const SSVM_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return (*Res)->getTableExports().size();
    }
  }
  return 0;
}

uint32_t SSVM_StoreListTableRegistered(SSVM_StoreContext *Cxt,
                                       const SSVM_String ModuleName,
                                       SSVM_String *Names, const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getTableExports(), Names, Len);
    }
  }
  return 0;
}

uint32_t SSVM_StoreListMemoryLength(const SSVM_StoreContext *Cxt) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getMemExports().size();
  }
  return 0;
}

uint32_t SSVM_StoreListMemory(const SSVM_StoreContext *Cxt, SSVM_String *Names,
                              const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getMemExports(), Names, Len);
  }
  return 0;
}

uint32_t SSVM_StoreListMemoryRegisteredLength(const SSVM_StoreContext *Cxt,
                                              const SSVM_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return (*Res)->getMemExports().size();
    }
  }
  return 0;
}

uint32_t SSVM_StoreListMemoryRegistered(SSVM_StoreContext *Cxt,
                                        const SSVM_String ModuleName,
                                        SSVM_String *Names,
                                        const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getMemExports(), Names, Len);
    }
  }
  return 0;
}

uint32_t SSVM_StoreListGlobalLength(const SSVM_StoreContext *Cxt) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getGlobalExports().size();
  }
  return 0;
}

uint32_t SSVM_StoreListGlobal(const SSVM_StoreContext *Cxt, SSVM_String *Names,
                              const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getGlobalExports(), Names, Len);
  }
  return 0;
}

uint32_t SSVM_StoreListGlobalRegisteredLength(const SSVM_StoreContext *Cxt,
                                              const SSVM_String ModuleName) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return (*Res)->getGlobalExports().size();
    }
  }
  return 0;
}

uint32_t SSVM_StoreListGlobalRegistered(SSVM_StoreContext *Cxt,
                                        const SSVM_String ModuleName,
                                        SSVM_String *Names,
                                        const uint32_t Len) {
  if (Cxt) {
    if (auto Res = fromStoreCxt(Cxt)->findModule(genStrView(ModuleName))) {
      return fillMap((*Res)->getGlobalExports(), Names, Len);
    }
  }
  return 0;
}

uint32_t SSVM_StoreListModuleLength(const SSVM_StoreContext *Cxt) {
  if (Cxt) {
    return fromStoreCxt(Cxt)->getModuleList().size();
  }
  return 0;
}

uint32_t SSVM_StoreListModule(SSVM_StoreContext *Cxt, SSVM_String *Names,
                              const uint32_t Len) {
  if (Cxt) {
    return fillMap(fromStoreCxt(Cxt)->getModuleList(), Names, Len);
  }
  return 0;
}

void SSVM_StoreDelete(SSVM_StoreContext *Cxt) { deleteIf(fromStoreCxt(Cxt)); }

/// <<<<<<<< SSVM store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM function type functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_FunctionTypeContext *SSVM_FunctionTypeCreate(
    const enum SSVM_ValType *ParamList, const uint32_t ParamLen,
    const enum SSVM_ValType *ReturnList, const uint32_t ReturnLen) {
  auto *Cxt = new SSVM::Runtime::Instance::FType;
  if (ParamLen > 0) {
    Cxt->Params.resize(ParamLen);
  }
  for (uint32_t I = 0; I < ParamLen; I++) {
    Cxt->Params[I] = static_cast<SSVM::ValType>(ParamList[I]);
  }
  if (ReturnLen > 0) {
    Cxt->Returns.resize(ReturnLen);
  }
  for (uint32_t I = 0; I < ReturnLen; I++) {
    Cxt->Returns[I] = static_cast<SSVM::ValType>(ReturnList[I]);
  }
  return toFTypeCxt(Cxt);
}

uint32_t
SSVM_FunctionTypeGetParametersLength(const SSVM_FunctionTypeContext *Cxt) {
  if (Cxt) {
    return fromFTypeCxt(Cxt)->Params.size();
  }
  return 0;
}

uint32_t SSVM_FunctionTypeGetParameters(const SSVM_FunctionTypeContext *Cxt,
                                        SSVM_ValType *List,
                                        const uint32_t Len) {
  if (Cxt) {
    for (uint32_t I = 0; I < fromFTypeCxt(Cxt)->Params.size() && I < Len; I++) {
      List[I] = static_cast<SSVM_ValType>(fromFTypeCxt(Cxt)->Params[I]);
    }
    return fromFTypeCxt(Cxt)->Params.size();
  }
  return 0;
}

uint32_t
SSVM_FunctionTypeGetReturnsLength(const SSVM_FunctionTypeContext *Cxt) {
  if (Cxt) {
    return fromFTypeCxt(Cxt)->Returns.size();
  }
  return 0;
}

uint32_t SSVM_FunctionTypeGetReturns(const SSVM_FunctionTypeContext *Cxt,
                                     SSVM_ValType *List, const uint32_t Len) {
  if (Cxt) {
    for (uint32_t I = 0; I < fromFTypeCxt(Cxt)->Returns.size() && I < Len;
         I++) {
      List[I] = static_cast<SSVM_ValType>(fromFTypeCxt(Cxt)->Returns[I]);
    }
    return fromFTypeCxt(Cxt)->Returns.size();
  }
  return 0;
}

void SSVM_FunctionTypeDelete(SSVM_FunctionTypeContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM function type functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM function instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

const SSVM_FunctionTypeContext *
SSVM_FunctionInstanceGetFunctionType(const SSVM_FunctionInstanceContext *Cxt) {
  if (Cxt) {
    return toFTypeCxt(&fromFuncCxt(Cxt)->getFuncType());
  }
  return nullptr;
}

/// <<<<<<<< SSVM function instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM host function functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_HostFunctionContext *
SSVM_HostFunctionCreate(const SSVM_FunctionTypeContext *Type,
                        SSVM_HostFunc_t HostFunc, const uint64_t Cost) {
  if (Type && HostFunc) {
    return toHostFuncCxt(new CAPIHostFunc(fromFTypeCxt(Type), HostFunc, Cost));
  }
  return nullptr;
}

SSVM_HostFunctionContext *
SSVM_HostFunctionCreateBinding(const SSVM_FunctionTypeContext *Type,
                               SSVM_WrapFunc_t WrapFunc, void *Binding,
                               const uint64_t Cost) {
  if (Type && WrapFunc) {
    return toHostFuncCxt(
        new CAPIHostFunc(fromFTypeCxt(Type), WrapFunc, Binding, Cost));
  }
  return nullptr;
}

void SSVM_HostFunctionDelete(SSVM_HostFunctionContext *Cxt) {
  deleteIf(fromHostFuncCxt(Cxt));
}

/// <<<<<<<< SSVM host function functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM table instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_TableInstanceContext *
SSVM_TableInstanceCreate(const enum SSVM_RefType RefType,
                         const SSVM_Limit Limit) {
  SSVM::RefType Type = static_cast<SSVM::RefType>(RefType);
  if (Limit.HasMax) {
    return toTabCxt(new SSVM::Runtime::Instance::TableInstance(
        Type, SSVM::AST::Limit(Limit.Min, Limit.Max)));
  } else {
    return toTabCxt(new SSVM::Runtime::Instance::TableInstance(
        Type, SSVM::AST::Limit(Limit.Min)));
  }
}

enum SSVM_RefType
SSVM_TableInstanceGetRefType(const SSVM_TableInstanceContext *Cxt) {
  if (Cxt) {
    return static_cast<SSVM_RefType>(fromTabCxt(Cxt)->getReferenceType());
  }
  return SSVM_RefType_FuncRef;
}

SSVM_Result SSVM_TableInstanceGetData(const SSVM_TableInstanceContext *Cxt,
                                      SSVM_Value *Data, const uint32_t Offset) {
  return wrap([&]() { return fromTabCxt(Cxt)->getRefAddr(Offset); },
              [&](auto &&Res) {
                *Data = genSSVM_Value(*Res,
                                      static_cast<SSVM_ValType>(
                                          fromTabCxt(Cxt)->getReferenceType()));
              },
              Cxt, Data);
}

SSVM_Result SSVM_TableInstanceSetData(SSVM_TableInstanceContext *Cxt,
                                      SSVM_Value Data, const uint32_t Offset) {
  return wrap(
      [&]() -> SSVM::Expect<void> {
        SSVM::RefType expType = fromTabCxt(Cxt)->getReferenceType();
        if (expType != static_cast<SSVM::RefType>(Data.Type)) {
          LOG(ERROR) << SSVM::ErrCode::RefTypeMismatch;
          LOG(ERROR) << SSVM::ErrInfo::InfoMismatch(
              static_cast<SSVM::ValType>(expType),
              static_cast<SSVM::ValType>(Data.Type));
          return Unexpect(SSVM::ErrCode::RefTypeMismatch);
        }
        return fromTabCxt(Cxt)->setRefAddr(
            Offset, std::get<SSVM::RefVariant>(SSVM::ValVariant(Data.Value)));
      },
      EmptyThen, Cxt);
}

uint32_t SSVM_TableInstanceGetSize(const SSVM_TableInstanceContext *Cxt) {
  if (Cxt) {
    return fromTabCxt(Cxt)->getSize();
  }
  return 0;
}

SSVM_Result SSVM_TableInstanceGrow(SSVM_TableInstanceContext *Cxt,
                                   const uint32_t Size) {
  return wrap(
      [&]() -> SSVM::Expect<void> {
        if (fromTabCxt(Cxt)->growTable(Size)) {
          return {};
        } else {
          return SSVM::Unexpect(SSVM::ErrCode::TableOutOfBounds);
        }
      },
      EmptyThen, Cxt);
}

void SSVM_TableInstanceDelete(SSVM_TableInstanceContext *Cxt) {
  deleteIf(fromTabCxt(Cxt));
}

/// <<<<<<<< SSVM table instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM memory instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_MemoryInstanceContext *SSVM_MemoryInstanceCreate(const SSVM_Limit Limit) {
  if (Limit.HasMax) {
    return toMemCxt(new SSVM::Runtime::Instance::MemoryInstance(
        SSVM::AST::Limit(Limit.Min, Limit.Max)));
  } else {
    return toMemCxt(new SSVM::Runtime::Instance::MemoryInstance(
        SSVM::AST::Limit(Limit.Min)));
  }
}

SSVM_Result SSVM_MemoryInstanceGetData(const SSVM_MemoryInstanceContext *Cxt,
                                       uint8_t *Data, const uint32_t Offset,
                                       const uint32_t Length) {
  return wrap([&]() { return fromMemCxt(Cxt)->getBytes(Offset, Length); },
              [&](auto &&Res) { std::copy_n((*Res).begin(), Length, Data); },
              Cxt, Data);
}

SSVM_Result SSVM_MemoryInstanceSetData(SSVM_MemoryInstanceContext *Cxt,
                                       uint8_t *Data, const uint32_t Offset,
                                       const uint32_t Length) {

  return wrap(
      [&]() {
        return fromMemCxt(Cxt)->setBytes(genSpan(Data, Length), Offset, 0,
                                         Length);
      },
      EmptyThen, Cxt, Data);
}

uint32_t SSVM_MemoryInstanceGetPageSize(const SSVM_MemoryInstanceContext *Cxt) {
  if (Cxt) {
    return fromMemCxt(Cxt)->getDataPageSize();
  }
  return 0;
}

SSVM_Result SSVM_MemoryInstanceGrowPage(SSVM_MemoryInstanceContext *Cxt,
                                        const uint32_t Page) {

  return wrap(
      [&]() -> SSVM::Expect<void> {
        if (fromMemCxt(Cxt)->growPage(Page)) {
          return {};
        } else {
          return SSVM::Unexpect(SSVM::ErrCode::MemoryOutOfBounds);
        }
      },
      EmptyThen, Cxt);
}

void SSVM_MemoryInstanceDelete(SSVM_MemoryInstanceContext *Cxt) {
  deleteIf(fromMemCxt(Cxt));
}

/// <<<<<<<< SSVM memory instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> SSVM global instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_GlobalInstanceContext *
SSVM_GlobalInstanceCreate(const SSVM_Value Value,
                          const enum SSVM_Mutability Mut) {
  return toGlobCxt(new SSVM::Runtime::Instance::GlobalInstance(
      static_cast<SSVM::ValType>(Value.Type), static_cast<SSVM::ValMut>(Mut),
      Value.Value));
}

enum SSVM_ValType
SSVM_GlobalInstanceGetValType(const SSVM_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return static_cast<SSVM_ValType>(fromGlobCxt(Cxt)->getValType());
  }
  return SSVM_ValType_I32;
}

enum SSVM_Mutability
SSVM_GlobalInstanceGetMutability(const SSVM_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return static_cast<SSVM_Mutability>(fromGlobCxt(Cxt)->getValMut());
  }
  return SSVM_Mutability_Const;
}

SSVM_Value SSVM_GlobalInstanceGetValue(const SSVM_GlobalInstanceContext *Cxt) {
  if (Cxt) {
    return genSSVM_Value(
        fromGlobCxt(Cxt)->getValue(),
        static_cast<SSVM_ValType>(fromGlobCxt(Cxt)->getValType()));
  }
  return genSSVM_Value(SSVM::ValVariant(static_cast<unsigned __int128>(0)),
                       SSVM_ValType_I32);
}

void SSVM_GlobalInstanceSetValue(SSVM_GlobalInstanceContext *Cxt,
                                 const SSVM_Value Value) {
  if (Cxt && fromGlobCxt(Cxt)->getValMut() == SSVM::ValMut::Var &&
      static_cast<SSVM::ValType>(Value.Type) ==
          fromGlobCxt(Cxt)->getValType()) {
    fromGlobCxt(Cxt)->getValue() = Value.Value;
  }
}

void SSVM_GlobalInstanceDelete(SSVM_GlobalInstanceContext *Cxt) {
  deleteIf(fromGlobCxt(Cxt));
}

/// <<<<<<<< SSVM global instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// <<<<<<<< SSVM import object functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

SSVM_ImportObjectContext *SSVM_ImportObjectCreate(const SSVM_String ModuleName,
                                                  void *Data) {
  return toImpObjCxt(new CAPIImportModule(ModuleName, Data));
}

SSVM_ImportObjectContext *SSVM_ImportObjectCreateWASI(
    const char *const *Args, const uint32_t ArgLen, const char *const *Envs,
    const uint32_t EnvLen, const char *const *Dirs, const uint32_t DirLen,
    const char *const *Preopens, const uint32_t PreopenLen) {
  auto *WasiMod = new SSVM::Host::WasiModule();
  SSVM_ImportObjectInitWASI(toImpObjCxt(WasiMod), Args, ArgLen, Envs, EnvLen,
                            Dirs, DirLen, Preopens, PreopenLen);
  return toImpObjCxt(WasiMod);
}

void SSVM_ImportObjectInitWASI(SSVM_ImportObjectContext *Cxt,
                               const char *const *Args, const uint32_t ArgLen,
                               const char *const *Envs, const uint32_t EnvLen,
                               const char *const *Dirs, const uint32_t DirLen,
                               const char *const *Preopens,
                               const uint32_t PreopenLen) {
  if (!Cxt) {
    return;
  }
  auto *WasiMod = dynamic_cast<SSVM::Host::WasiModule *>(fromImpObjCxt(Cxt));
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

void SSVM_ImportObjectAddHostFunction(SSVM_ImportObjectContext *Cxt,
                                      const SSVM_String Name,
                                      SSVM_HostFunctionContext *HostFuncCxt) {
  if (Cxt && HostFuncCxt) {
    auto *ImpMod = reinterpret_cast<CAPIImportModule *>(Cxt);
    auto *HostFunc = reinterpret_cast<CAPIHostFunc *>(HostFuncCxt);
    HostFunc->setData(ImpMod->getData());
    fromImpObjCxt(Cxt)->addHostFunc(
        genStrView(Name),
        std::unique_ptr<SSVM::Runtime::HostFunctionBase>(HostFunc));
  }
}

void SSVM_ImportObjectAddTable(SSVM_ImportObjectContext *Cxt,
                               const SSVM_String Name,
                               SSVM_TableInstanceContext *TableCxt) {
  if (Cxt && TableCxt) {
    fromImpObjCxt(Cxt)->addHostTable(
        genStrView(Name),
        std::unique_ptr<SSVM::Runtime::Instance::TableInstance>(
            fromTabCxt(TableCxt)));
  }
}

void SSVM_ImportObjectAddMemory(SSVM_ImportObjectContext *Cxt,
                                const SSVM_String Name,
                                SSVM_MemoryInstanceContext *MemoryCxt) {
  if (Cxt && MemoryCxt) {
    fromImpObjCxt(Cxt)->addHostMemory(
        genStrView(Name),
        std::unique_ptr<SSVM::Runtime::Instance::MemoryInstance>(
            fromMemCxt(MemoryCxt)));
  }
}

void SSVM_ImportObjectAddGlobal(SSVM_ImportObjectContext *Cxt,
                                const SSVM_String Name,
                                SSVM_GlobalInstanceContext *GlobalCxt) {
  if (Cxt && GlobalCxt) {
    fromImpObjCxt(Cxt)->addHostGlobal(
        genStrView(Name),
        std::unique_ptr<SSVM::Runtime::Instance::GlobalInstance>(
            fromGlobCxt(GlobalCxt)));
  }
}

void SSVM_ImportObjectDelete(SSVM_ImportObjectContext *Cxt) {
  deleteIf(fromImpObjCxt(Cxt));
}

/// >>>>>>>> SSVM import object functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// >>>>>>>> SSVM VM functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SSVM_VMContext *SSVM_VMCreate(const SSVM_ConfigureContext *ConfCxt,
                              SSVM_StoreContext *StoreCxt) {
  if (ConfCxt) {
    if (StoreCxt) {
      return new SSVM_VMContext(ConfCxt->Conf, *fromStoreCxt(StoreCxt));
    } else {
      return new SSVM_VMContext(ConfCxt->Conf);
    }
  } else {
    if (StoreCxt) {
      return new SSVM_VMContext(SSVM::Configure(), *fromStoreCxt(StoreCxt));
    } else {
      return new SSVM_VMContext(SSVM::Configure());
    }
  }
}

SSVM_Result SSVM_VMRegisterModuleFromFile(SSVM_VMContext *Cxt,
                                          const SSVM_String ModuleName,
                                          const char *Path) {
  return wrap(
      [&]() { return Cxt->VM.registerModule(genStrView(ModuleName), Path); },
      EmptyThen, Cxt);
}

SSVM_Result SSVM_VMRegisterModuleFromBuffer(SSVM_VMContext *Cxt,
                                            const SSVM_String ModuleName,
                                            const uint8_t *Buf,
                                            const uint32_t BufLen) {
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      genSpan(Buf, BufLen));
      },
      EmptyThen, Cxt);
}

SSVM_Result
SSVM_VMRegisterModuleFromImport(SSVM_VMContext *Cxt,
                                const SSVM_ImportObjectContext *ImportCxt) {
  return wrap(
      [&]() { return Cxt->VM.registerModule(*fromImpObjCxt(ImportCxt)); },
      EmptyThen, Cxt, ImportCxt);
}

SSVM_Result
SSVM_VMRegisterModuleFromASTModule(SSVM_VMContext *Cxt,
                                   const SSVM_String ModuleName,
                                   const SSVM_ASTModuleContext *ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->VM.registerModule(genStrView(ModuleName),
                                      *ASTCxt->Module.get());
      },
      EmptyThen, Cxt, ASTCxt);
}

SSVM_Result SSVM_VMRunWasmFromFile(SSVM_VMContext *Cxt, const char *Path,
                                   const SSVM_String FuncName,
                                   const SSVM_Value *Params,
                                   const uint32_t ParamLen, SSVM_Value *Returns,
                                   const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(Path, genStrView(FuncName), ParamPair.first,
                                   ParamPair.second);
      },
      [&](auto Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

SSVM_Result
SSVM_VMRunWasmFromBuffer(SSVM_VMContext *Cxt, const uint8_t *Buf,
                         const uint32_t BufLen, const SSVM_String FuncName,
                         const SSVM_Value *Params, const uint32_t ParamLen,
                         SSVM_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(genSpan(Buf, BufLen), genStrView(FuncName),
                                   ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

SSVM_Result SSVM_VMRunWasmFromASTModule(
    SSVM_VMContext *Cxt, const SSVM_ASTModuleContext *ASTCxt,
    const SSVM_String FuncName, const SSVM_Value *Params,
    const uint32_t ParamLen, SSVM_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.runWasmFile(*ASTCxt->Module.get(), genStrView(FuncName),
                                   ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt,
      ASTCxt);
}

SSVM_Result SSVM_VMLoadWasmFromFile(SSVM_VMContext *Cxt, const char *Path) {
  return wrap([&]() { return Cxt->VM.loadWasm(Path); }, EmptyThen, Cxt);
}

SSVM_Result SSVM_VMLoadWasmFromBuffer(SSVM_VMContext *Cxt, const uint8_t *Buf,
                                      const uint32_t BufLen) {
  return wrap([&]() { return Cxt->VM.loadWasm(genSpan(Buf, BufLen)); },
              EmptyThen, Cxt);
}

SSVM_Result SSVM_VMLoadWasmFromASTModule(SSVM_VMContext *Cxt,
                                         const SSVM_ASTModuleContext *ASTCxt) {
  return wrap([&]() { return Cxt->VM.loadWasm(*ASTCxt->Module.get()); },
              EmptyThen, Cxt, ASTCxt);
}

SSVM_Result SSVM_VMValidate(SSVM_VMContext *Cxt) {
  return wrap([&]() { return Cxt->VM.validate(); }, EmptyThen, Cxt);
}

SSVM_Result SSVM_VMInstantiate(SSVM_VMContext *Cxt) {
  return wrap([&]() { return Cxt->VM.instantiate(); }, EmptyThen, Cxt);
}

SSVM_Result SSVM_VMExecute(SSVM_VMContext *Cxt, const SSVM_String FuncName,
                           const SSVM_Value *Params, const uint32_t ParamLen,
                           SSVM_Value *Returns, const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.execute(genStrView(FuncName), ParamPair.first,
                               ParamPair.second);
      },
      [&](auto &&Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

SSVM_Result
SSVM_VMExecuteRegistered(SSVM_VMContext *Cxt, const SSVM_String ModuleName,
                         const SSVM_String FuncName, const SSVM_Value *Params,
                         const uint32_t ParamLen, SSVM_Value *Returns,
                         const uint32_t ReturnLen) {
  auto ParamPair = genParamPair(Params, ParamLen);
  return wrap(
      [&]() {
        return Cxt->VM.execute(genStrView(ModuleName), genStrView(FuncName),
                               ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { fillSSVM_ValueArr(*Res, Returns, ReturnLen); }, Cxt);
}

SSVM_FunctionTypeContext *SSVM_VMGetFunctionType(SSVM_VMContext *Cxt,
                                                 const SSVM_String FuncName) {
  if (Cxt) {
    const auto FuncList = Cxt->VM.getFunctionList();
    for (const auto &It : FuncList) {
      if (It.first == genStrView(FuncName)) {
        return toFTypeCxt(new SSVM::Runtime::Instance::FType(It.second));
      }
    }
  }
  return nullptr;
}

SSVM_FunctionTypeContext *
SSVM_VMGetFunctionTypeRegistered(SSVM_VMContext *Cxt,
                                 const SSVM_String ModuleName,
                                 const SSVM_String FuncName) {
  if (Cxt) {
    auto &Store = Cxt->VM.getStoreManager();
    if (auto Res = Store.findModule(genStrView(ModuleName))) {
      const auto *ModInst = *Res;
      const auto &FuncExp = ModInst->getFuncExports();
      const auto FuncIter = FuncExp.find(genStrView(FuncName));
      if (FuncIter != FuncExp.cend()) {
        const auto *FuncInst = *Store.getFunction(FuncIter->second);
        return toFTypeCxt(
            new SSVM::Runtime::Instance::FType(FuncInst->getFuncType()));
      }
    }
  }
  return nullptr;
}

void SSVM_VMCleanup(SSVM_VMContext *Cxt) {
  if (Cxt) {
    Cxt->VM.cleanup();
  }
}

uint32_t SSVM_VMGetFunctionListLength(SSVM_VMContext *Cxt) {
  if (Cxt) {
    return Cxt->VM.getFunctionList().size();
  }
  return 0;
}

uint32_t SSVM_VMGetFunctionList(SSVM_VMContext *Cxt, SSVM_String *Names,
                                SSVM_FunctionTypeContext **FuncTypes,
                                const uint32_t Len) {
  if (Cxt) {
    auto FuncList = Cxt->VM.getFunctionList();
    for (uint32_t I = 0; I < Len && I < FuncList.size(); I++) {
      if (Names) {
        uint32_t NameLen = FuncList[I].first.length();
        char *Str = new char[NameLen];
        std::copy_n(&FuncList[I].first.data()[0], NameLen, Str);
        Names[I] = SSVM_String{.Length = NameLen, .Buf = Str};
      }
      if (FuncTypes) {
        FuncTypes[I] =
            toFTypeCxt(new SSVM::Runtime::Instance::FType(FuncList[I].second));
      }
    }
    return FuncList.size();
  }
  return 0;
}

SSVM_ImportObjectContext *
SSVM_VMGetImportModuleContext(SSVM_VMContext *Cxt,
                              const enum SSVM_HostRegistration Reg) {
  if (Cxt) {
    return toImpObjCxt(
        Cxt->VM.getImportModule(static_cast<SSVM::HostRegistration>(Reg)));
  }
  return nullptr;
}

SSVM_StoreContext *SSVM_VMGetStoreContext(SSVM_VMContext *Cxt) {
  if (Cxt) {
    return toStoreCxt(&Cxt->VM.getStoreManager());
  }
  return nullptr;
}

SSVM_StatisticsContext *SSVM_VMGetStatisticsContext(SSVM_VMContext *Cxt) {
  if (Cxt) {
    return toStatCxt(&Cxt->VM.getStatistics());
  }
  return nullptr;
}

void SSVM_VMDelete(SSVM_VMContext *Cxt) { deleteIf(Cxt); }

/// <<<<<<<< SSVM VM functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif
