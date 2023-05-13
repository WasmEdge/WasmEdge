#include "wasmedge/wasmedge.hh"

#include "driver/compiler.h"
#include "driver/tool.h"
#include "vm/vm.h"

namespace {

// Helper function for returning a struct uint128_t / int128_t
// from class WasmEdge::uint128_t / WasmEdge::int128_t.
template <typename C>
inline constexpr ::uint128_t to_uint128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return {.Low = Val.low(), .High = static_cast<uint64_t>(Val.high())};
#endif
}
template <typename C> inline constexpr ::int128_t to_int128_t(C Val) noexcept {
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
  return Val;
#else
  return {.Low = Val.low(), .High = Val.high()};
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

// Helper template to run and return result
auto EmptyThen = [](auto &&) noexcept {};
template <typename T, typename U, typename ...CxtT>
inline constexpr WasmEdge::SDK::Result wrap(
    T &&Proc, U &&Then, CxtT &...Cxts) noexcept {
  if (auto Res = Proc()) {
    Then(Res);
    return WasmEdge::SDK::Result::ResultFactory::GenResult(
        WasmEdge::ErrCode::Value::Success);
  }
  return WasmEdge::SDK::Result::ResultFactory::GenResult(Res.error());
}

}

namespace WasmEdge {
namespace SDK {

// >>>>>>>> WasmEdge Context members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class ASTModule::ASTModuleContext: public WasmEdge::AST::Module {};

class FunctionType::FunctionTypeContext: public WasmEdge::AST::FunctionType {
public:
  FunctionTypeContext(AST::FunctionType &Func)
  : AST::FunctionType(Func) {}
};

class TableType::TableTypeContext: public WasmEdge::AST::TableType {
public:
  TableTypeContext(AST::TableType &TabType): AST::TableType(TabType) {}
};

class MemoryType::MemoryTypeContext: public WasmEdge::AST::MemoryType {
public:
  MemoryTypeContext(AST::MemoryType &MemType): AST::MemoryType(MemType) {}
};

class GlobalType::GlobalTypeContext: public WasmEdge::AST::GlobalType {
public:
  GlobalTypeContext(AST::GlobalType &GlobType)
  : WasmEdge::AST::GlobalType(GlobType) {}
};

class ImportType::ImportTypeContext: public WasmEdge::AST::ImportDesc {};
class ExportType::ExportTypeContext: public WasmEdge::AST::ExportDesc {};

class Async::AsyncContext: public WasmEdge::VM::Async<
  WasmEdge::Expect<
      std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>> {
  template <typename... Args>
  AsyncContext(Args &&...Vals)
  : WasmEdge::VM::Async(std::forward<Args>(Vals)...) {}
};

class Configuration::ConfigureContext: public WasmEdge::Configure {};
class Statistics::StatisticsContext: public WasmEdge::Statistics::Statistics {};

class Loader::LoaderContext: public WasmEdge::Loader::Loader {};

class Validator::ValidatorContext: public WasmEdge::Validator::Validator {
public:
  ValidatorContext(WasmEdge::Configure &Conf)
  : WasmEdge::Validator::Validator(Conf) {};
};

class Executor::ExecutorContext: public WasmEdge::Executor::Executor {
public:
  ExecutorContext(WasmEdge::Configure &Conf,
                  WasmEdge::Statistics::Statistics *Stat)
  : WasmEdge::Executor::Executor(Conf, Stat) {}
};

class Store::StoreContext: public WasmEdge::Runtime::StoreManager {};
class FunctionInstance::FunctionInstanceContext
: public WasmEdge::Runtime::Instance::FunctionInstance {};
class ModuleInstance::ModuleInstanceContext
: public WasmEdge::Runtime::Instance::ModuleInstance {};
class VM::VMContext: public WasmEdge::VM::VM {};

// <<<<<<<< WasmEdge Context members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> . Version members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

std::string Version::Get() {
  return WASMEDGE_VERSION;
}

uint32_t Version::GetMajor() {
  return WASMEDGE_VERSION_MAJOR;
}

uint32_t Version::GetMinor() {
  return WASMEDGE_VERSION_MINOR;
}

uint32_t Version::GetPatch() {
  return WASMEDGE_VERSION_PATCH;
}

// <<<<<<<< WasmEdge Version members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Log members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void Log::SetErrorLevel() {
  WasmEdge::Log::setErrorLoggingLevel();
}

void Log::SetDebugLevel() {
  WasmEdge::Log::setDebugLoggingLevel();
}

void Log::Off() {
  WasmEdge::Log::setLogOff();
}

// <<<<<<<< WasmEdge Log members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Value members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class Value::ValueUtils {
  ValueUtils() = default;
  ~ValueUtils() = default;

public:
  // Helper function for converting a WasmEdge_Value array to a ValVariant
  // vector.
  static std::pair<std::vector<WasmEdge::ValVariant>,
  std::vector<WasmEdge::ValType>> GenParamPair(
      const std::vector<WasmEdge::SDK::Value> &Val) noexcept {

    std::vector<WasmEdge::ValVariant> VVec;
    std::vector<WasmEdge::ValType> TVec;

    auto Len = Val.size();
    VVec.resize(Len);
    TVec.resize(Len);
    for (uint32_t I = 0; I < Len; I++) {
      TVec[I] = static_cast<WasmEdge::ValType>(Val[I].Type);
      switch (TVec[I]) {
      case WasmEdge::ValType::I32:
        VVec[I] = ValVariant::wrap<uint32_t>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::I64:
        VVec[I] = ValVariant::wrap<uint64_t>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::F32:
        VVec[I] = ValVariant::wrap<float>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::F64:
        VVec[I] = ValVariant::wrap<double>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::V128:
        VVec[I] = ValVariant::wrap<WasmEdge::uint128_t>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::FuncRef:
        VVec[I] = ValVariant::wrap<WasmEdge::FuncRef>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      case WasmEdge::ValType::ExternRef:
        VVec[I] = ValVariant::wrap<WasmEdge::ExternRef>(
            to_WasmEdge_128_t<WasmEdge::uint128_t>(Val[I].Val));
        break;
      default:
        // TODO: Return error
        assumingUnreachable();
      }
    }
    return {VVec, TVec};
  }

  static inline void FillValueArr(
      Span<const std::pair<ValVariant, WasmEdge::ValType>> Vec,
      std::vector<Value> &Returns) {
    Returns.resize(Vec.size());
    for (uint32_t I = 0; I < Vec.size(); I++) {
      Returns[I] = std::move(Value(to_uint128_t(Vec[I].first.unwrap()),
                            static_cast<ValType>(Vec[I].second)));
    }
  }
};

Value::Value(const int32_t Val)
: Val(to_uint128_t(Val)),
  Type(ValType::I32) {}

Value::Value(const int64_t Val)
: Val(to_uint128_t(Val)),
  Type(ValType::I64) {}

Value::Value(const float Val)
: Val(to_uint128_t(Val)),
  Type(ValType::F32) {}

Value::Value(const double Val)
: Val(to_uint128_t(Val)),
  Type(ValType::F64) {}

Value::Value(const ::int128_t Val)
: Val(to_WasmEdge_128_t<WasmEdge::int128_t>(Val)),
  Type(static_cast<ValType>(
    WasmEdge::ValTypeFromType<::int128_t>())) {}

Value::Value(const RefType ValT)
: Val(to_uint128_t(WasmEdge::UnknownRef())),
  Type(static_cast<ValType>(ValT)) {}

Value::Value(const FunctionInstance &Cxt) {
  // TODO: Implement FunctionInstance functions
}

Value::Value(std::shared_ptr<void> ExtRef)
: Val(to_uint128_t(WasmEdge::ExternRef(ExtRef.get()))),
  Type(ValType::ExternRef) {}

int32_t Value::GetI32() {
  return WasmEdge::ValVariant::wrap<int32_t>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<int32_t>();
}

int64_t Value::GetI64() {
  return WasmEdge::ValVariant::wrap<int64_t>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<int64_t>();
}

float Value::GetF32() {
  return WasmEdge::ValVariant::wrap<float>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<float>();
}

double Value::GetF64() {
  return WasmEdge::ValVariant::wrap<double>(
             to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
      .get<double>();
}

int128_t Value::GetV128() {
  return to_int128_t(WasmEdge::ValVariant::wrap<WasmEdge::int128_t>(
                         to_WasmEdge_128_t<WasmEdge::uint128_t>(Val))
                         .get<WasmEdge::int128_t>());
}

const FunctionInstance &Value::GetFuncRef() {
  // TODO: Implement FunctionInstance constructor
}

std::shared_ptr<void> Value::GetExternRef() {
  auto ExternRef = WasmEdge::retrieveExternRef<uint32_t>(
      WasmEdge::ValVariant::wrap<WasmEdge::ExternRef>(
          to_WasmEdge_128_t<WasmEdge::uint128_t>(Val)));
  return std::make_shared<void>(ExternRef);
}

bool Value::IsNullRef() {
  return WasmEdge::isNullRef(WasmEdge::ValVariant::wrap<WasmEdge::UnknownRef>(
      to_WasmEdge_128_t<WasmEdge::uint128_t>(Val)));
}

// <<<<<<<< WasmEdge Value members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Result members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class Result::ResultFactory {
  ResultFactory() = default;
  ~ResultFactory() = default;

public:
  static Result GenResult(const WasmEdge::ErrCode::Value &Code) {
    return Result{ static_cast<uint32_t>(Code) & 0x00FFFFFFU };
  }

  static Result GenResult(const WasmEdge::ErrCode &Code) {
    return Result{ Code.operator uint32_t() };
  }
};

Result::Result(const ErrCategory Category, const uint32_t Code) {
  this->Code = (static_cast<uint32_t>(Category) << 24) + (Code & 0x00FFFFFFU);
}

bool Result::IsOk() {
  if (GetCategory() == ErrCategory::WASM &&
      (static_cast<WasmEdge::ErrCode::Value>(GetCode()) ==
           WasmEdge::ErrCode::Value::Success ||
       static_cast<WasmEdge::ErrCode::Value>(GetCode()) ==
           WasmEdge::ErrCode::Value::Terminated)) {
    return true;
  } else {
    return false;
  }
}

uint32_t Result::GetCode() {
  return Code & 0x00FFFFFFU;
}

ErrCategory Result::GetCategory() {
  return static_cast<ErrCategory>(Code >> 24);
}

const std::string Result::GetMessage() {
  if (GetCategory() != ErrCategory::WASM) {
    auto str = WasmEdge::ErrCodeStr[WasmEdge::ErrCode::Value::UserDefError];
    return std::string{ str };
  }
  auto str = WasmEdge::ErrCodeStr[static_cast<WasmEdge::ErrCode::Value>(
                                  GetCode())];
  return std::string{ str };
}

// <<<<<<<< WasmEdge Result members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool Limit::operator==(const Limit &Lim) {
  return this->HasMax == Lim.HasMax && this->Shared == Lim.Shared &&
         this->Min == Lim.Min && this->Max == Lim.Max;
}

// >>>>>>>> WasmEdge FunctionType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

FunctionType::FunctionType(const std::vector<ValType> &ParamList,
                           const std::vector<ValType> &ReturnList) {
  auto FuncTypeCxt = std::make_unique<FunctionTypeContext>();
  if (!ParamList.empty()) {
    FuncTypeCxt->getParamTypes().reserve(ParamList.size());
    std::copy(ParamList.begin(), ParamList.end(),
              std::back_inserter(ParamList));
  }
  if (!ReturnList.empty()) {
    FuncTypeCxt->getReturnTypes().reserve(ReturnList.size());
    std::copy(ReturnList.begin(), ReturnList.end(),
              std::back_inserter(ReturnList));
  }

  this->Cxt = std::move(FuncTypeCxt);
}

const std::vector<ValType> FunctionType::GetParameters() {
  std::vector<ValType> OutputParams;
  auto FuncParams = Cxt->getParamTypes();
  OutputParams.resize(FuncParams.size());
  std::transform(FuncParams.begin(), FuncParams.end(),
                 OutputParams.begin(),
                 [](auto ValT) {return static_cast<ValType>(ValT)});
  return OutputParams;
}

const std::vector<ValType> FunctionType::GetReturns() {
  std::vector<ValType> OutputReturns;
  auto FuncReturns = Cxt->getReturnTypes();
  OutputReturns.resize(FuncReturns.size());
  std::transform(FuncReturns.begin(), FuncReturns.end(),
                 OutputReturns.begin(),
                 [](auto ValT) {return static_cast<ValType>(ValT)});
  return OutputReturns;
}

// <<<<<<<< WasmEdge FunctionType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge TableType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

TableType::TableType(const RefType RefT, const Limit &Lim) {
  if (Lim.HasMax) {
    this->Cxt = std::make_unique<TableType::TableTypeContext>(
                                 RefT, Lim.Min, Lim.Max);
  } else {
    this->Cxt = std::make_unique<TableType::TableTypeContext>(
                          RefT, Lim.Min);
  }
}

RefType TableType::GetRefType() {
  return static_cast<RefType>(Cxt->getRefType());
}

const Limit &TableType::GetLimit() {
  const auto &Lim = Cxt->getLimit();
  auto TableLim =  std::make_unique<Limit>(Lim.hasMax(), Lim.isShared(),
                                  Lim.getMin(), Lim.getMax());
  return std::move(*TableLim);
}

// <<<<<<<< WasmEdge TableType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge MemoryType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MemoryType::MemoryType(const Limit &Lim) {
  if (Lim.Shared) {
    this->Cxt = std::make_unique<MemoryType::MemoryTypeContext>(
                                 Lim.Min, Lim.Max, true);
  } else if (Lim.HasMax) {
    this->Cxt = std::make_unique<MemoryType::MemoryTypeContext>(
                                 Lim.Min, Lim.Max);
  } else {
    this->Cxt = std::make_unique<MemoryType::MemoryTypeContext>(Lim.Min);
  }
}

const Limit &MemoryType::GetLimit() {
  const auto &Lim = Cxt->getLimit();
  auto MemoryLim = std::make_unique<Limit>(Lim.hasMax(), Lim.isShared(),
                                           Lim.getMin(), Lim.getMax());
  return std::move(*MemoryLim);
}

// <<<<<<<< WasmEdge MemoryType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge GlobalType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

GlobalType::GlobalType(const ValType ValT, const Mutability Mut) {
  this->Cxt = std::make_unique<GlobalType::GlobalTypeContext>(ValT, Mut);
}

ValType GlobalType::GetValType() {
  return static_cast<ValType>(Cxt->getValType());
}

Mutability GlobalType::GetMutability() {
  return static_cast<Mutability>(Cxt->getValMut());
}

// <<<<<<<< WasmEdge GlobalType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ImportType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ImportType::ImportType(ImportType::ImportTypeContext &Cxt): Cxt(Cxt) {}

ExternalType ImportType::GetExternalType() {
  return static_cast<ExternalType>(Cxt.getExternalType());
}

std::string ImportType::GetModuleName() {
  std::string str{ Cxt.getModuleName() };
  return str;
}

std::string ImportType::GetExternalName() {
  std::string str{ Cxt.getExternalName() };
  return str;
}

std::shared_ptr<const FunctionType>
ImportType::GetFunctionType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Function) {
    uint32_t Idx = Cxt.getExternalFuncTypeIdx();
    const auto &FuncTypes = ASTCxt.Cxt->getTypeSection().getContent();
    if (Idx >= FuncTypes.size()) return nullptr;

    auto FuncTypeCxt =
        std::make_unique<FunctionType::FunctionTypeContext>(FuncTypes[Idx]);
    auto FuncType = std::shared_ptr<FunctionType>();
    FuncType->Cxt = std::move(FuncTypeCxt);
    return FuncType;
  }
  return nullptr;
}

std::shared_ptr<const TableType>
ImportType::GetTableType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Table) {
    auto TabTypeCxt =
        std::make_unique<TableType::TableTypeContext>(
          Cxt.getExternalTableType());
    auto TabType = std::shared_ptr<TableType>();
    TabType->Cxt = std::move(TabTypeCxt);
    return TabType;
  }
  return nullptr;
}

std::shared_ptr<const MemoryType>
ImportType::GetMemoryType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Memory) {
    auto ExternalMemType = Cxt.getExternalMemoryType();
    auto MemTypeCxt =
        std::make_unique<MemoryType::MemoryTypeContext>(ExternalMemType);
    auto MemType = std::shared_ptr<MemoryType>();
    MemType->Cxt = std::move(MemTypeCxt);
    return MemType;
  }
  return nullptr;
}

std::shared_ptr<const GlobalType>
ImportType::GetGlobalType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Global) {
    auto ExternalGlobType = Cxt.getExternalGlobalType();
    auto GlobTypeCxt =
        std::make_unique<GlobalType::GlobalTypeContext>(ExternalGlobType);
    auto GlobType = std::shared_ptr<GlobalType>();
    GlobType->Cxt = std::move(GlobTypeCxt);
    return GlobType;
  }
  return nullptr;
}

// <<<<<<<< WasmEdge ImportType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ExportType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ExportType::ExportType(ExportType::ExportTypeContext &Cxt): Cxt(Cxt) {}

ExternalType ExportType::GetExternalType() {
  return static_cast<ExternalType>(Cxt.getExternalType());
}

std::string ExportType::GetExternalName() {
  std::string str{ Cxt.getExternalName() };
  return str;
}

std::shared_ptr<const FunctionType>
ExportType::GetFunctionType (const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Function) {
    // `external_index` = `func_index` + `import_func_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = ASTCxt.Cxt->getImportSection().getContent();
    for (auto &&ImpDesc: ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Function) {
        ExtIdx--;
      }
    }
    // Get the function type index by the function index
    const auto &FuncIdxs =
        ASTCxt.Cxt->getFunctionSection().getContent();
    if (ExtIdx >= FuncIdxs.size()) {
      return nullptr;
    }
    uint32_t TypeIdx = FuncIdxs[ExtIdx];
    // Get the function type
    const auto &FuncTypes =
        ASTCxt.Cxt->getTypeSection().getContent();
    if (TypeIdx >= FuncTypes.size()) {
      return nullptr;
    }

    auto FuncTypeCxt =
        std::make_unique<FunctionType::FunctionTypeContext>(FuncTypes[TypeIdx]);
    auto FuncType = std::shared_ptr<FunctionType>();
    FuncType->Cxt = std::move(FuncTypeCxt);
    return FuncType;
  }
  return nullptr;
}

std::shared_ptr<const TableType>
ExportType::GetTableType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Table) {
    // `external_index` = `table_type_index` + `import_table_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = ASTCxt.Cxt->getImportSection().getContent();
    for (auto &&ImpDesc: ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Table) {
        ExtIdx--;
      }
    }
    //Get the table type
    const auto &TabTypes = ASTCxt.Cxt->getTableSection().getContent();
    if (ExtIdx >= TabTypes.size()) {
      return nullptr;
    }
    auto TabTypeCxt =
        std::make_unique<TableType::TableTypeContext>(TabTypes[ExtIdx]);
    auto TabType = std::shared_ptr<TableType>();
    TabType->Cxt = std::move(TabTypeCxt);
    return TabType;
  }
  return nullptr;
}

std::shared_ptr<const MemoryType>
ExportType::GetMemoryType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Memory) {
    // `external_index` = `memory_type_index` + `import_memory_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = ASTCxt.Cxt->getImportSection().getContent();
    for (auto &&ImpDesc: ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Memory) {
        ExtIdx--;
      }
    }
    // Get the memory type
    const auto &MemTypes = ASTCxt.Cxt->getMemorySection().getContent();
    if (ExtIdx >= MemTypes.size()) {
      return nullptr;
    }
    auto MemTypeCxt =
        std::make_unique<MemoryType::MemoryTypeContext>(MemTypes[ExtIdx]);
    auto MemType = std::shared_ptr<MemoryType>();
    MemType->Cxt = std::move(MemTypeCxt);
    return MemType;
  }
  return nullptr;
}

std::shared_ptr<const GlobalType>
ExportType::GetGlobalType(const ASTModule &ASTCxt) {
  if (Cxt.getExternalType() == WasmEdge::ExternalType::Global) {
    // `external_index` = `global_type_index` + `import_global_nums`
    uint32_t ExtIdx = Cxt.getExternalIndex();
    const auto &ImpDescs = ASTCxt.Cxt->getImportSection().getContent();
    for (auto &&ImpDesc: ImpDescs) {
      if (ImpDesc.getExternalType() == WasmEdge::ExternalType::Global) {
        ExtIdx--;
      }
    }
    // Get the global type
    const auto &GlobDescs = ASTCxt.Cxt->getGlobalSection().getContent();
    if (ExtIdx >= GlobDescs.size()) {
      return nullptr;
    }
    auto GlobTypeCxt =
        std::make_unique<GlobalType::GlobalTypeContext>(
          GlobDescs[ExtIdx].getGlobalType());
    auto GlobType = std::shared_ptr<GlobalType>();
    GlobType->Cxt = std::move(GlobTypeCxt);
    return GlobType;
  }
  return nullptr;
}

// <<<<<<<< WasmEdge ExportType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Async >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

template <typename... Args>
Async::Async(Args &&...Vals) {
  this->Cxt = std::make_unique<AsyncContext>();
}

void Async::Wait() {
  return Cxt->wait();
}

bool Async::WaitFor(uint64_t Milliseconds) {
  return Cxt->waitFor(std::chrono::milliseconds(Milliseconds));  
}

void Async::Cancel() {
  Cxt->cancel();
}

Result Async::Get(std::vector<Value> &Returns) {
  auto Res = Cxt->get();
  if (Res) {
    for (uint32_t I = 0; I < Res->size(); I++) {
      auto Val = to_uint128_t(Res->at(I).first.unwrap());
      Returns[I] = Value(Val, static_cast<ValType>(Res->at(I).second));
    }
    return Result{
      static_cast<uint32_t>(ErrCode::Value::Success) & 0x00FFFFFFU};
  }
  return Result{
    static_cast<uint32_t>(Res.error()) & 0x00FFFFFFU};
}

// <<<<<<<< WasmEdge Async <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Configuration >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Configuration::Configuration() {
  this->Cxt = std::make_unique<Configuration::ConfigureContext>();
}

void Configuration::AddProposal(const Proposal Prop) {
  Cxt->addProposal(static_cast<WasmEdge::Proposal>(Prop));
}

void Configuration::RemoveProposal(const Proposal Prop) {
  Cxt->removeProposal(static_cast<WasmEdge::Proposal>(Prop));
}

bool Configuration::HasProposal(const Proposal Prop) {
  return Cxt->hasProposal(static_cast<WasmEdge::Proposal>(Prop));
}

void Configuration::AddHostRegistration(const HostRegistration Host) {
  Cxt->addHostRegistration(static_cast<WasmEdge::HostRegistration>(Host));
}

void Configuration::RemoveHostRegistration(const HostRegistration Host) {
  Cxt->removeHostRegistration(static_cast<WasmEdge::HostRegistration>(Host));
}

bool Configuration::HasHostRegistration(const HostRegistration Host) {
  return Cxt->hasHostRegistration(
      static_cast<WasmEdge::HostRegistration>(Host));
}

void Configuration::SetMaxMemoryPage(const uint32_t Page) {
  Cxt->getRuntimeConfigure().setMaxMemoryPage(Page);
}

uint32_t Configuration::GetMaxMemoryPage() {
  return Cxt->getRuntimeConfigure().getMaxMemoryPage();
}

void Configuration::SetForceInterpreter(const bool IsForceInterpreter) {
  Cxt->getRuntimeConfigure().setForceInterpreter(IsForceInterpreter);
}

bool Configuration::IsForceInterpreter() {
  return Cxt->getRuntimeConfigure().isForceInterpreter();
}

void Configuration::CompilerSetOptimizationLevel(
    const CompilerOptimizationLevel Level) {
  Cxt->getCompilerConfigure().setOptimizationLevel(
      static_cast<WasmEdge::CompilerConfigure::OptimizationLevel>(Level));
}

CompilerOptimizationLevel Configuration::CompilerGetOptimizationLevel() {
  return static_cast<CompilerOptimizationLevel>(
      Cxt->getCompilerConfigure().getOptimizationLevel());
}

void Configuration::CompilerSetOutputFormat(const CompilerOutputFormat Format) {
  Cxt->getCompilerConfigure().setOutputFormat(
      static_cast<WasmEdge::CompilerConfigure::OutputFormat>(Format));
}

CompilerOutputFormat Configuration::CompilerGetOutputFormat() {
  return static_cast<CompilerOutputFormat>(
      Cxt->getCompilerConfigure().getOutputFormat());
}

void Configuration::CompilerSetDumpIR(const bool IsDump) {
  Cxt->getCompilerConfigure().setDumpIR(IsDump);
}

bool Configuration::CompilerIsDumpIR() {
  return Cxt->getCompilerConfigure().isDumpIR();
}

void Configuration::CompilerSetGenericBinary(const bool IsGeneric) {
  Cxt->getCompilerConfigure().setGenericBinary(IsGeneric);
}

bool Configuration::CompilerIsGenericBinary() {
  return Cxt->getCompilerConfigure().isGenericBinary();
}

void Configuration::CompilerSetInterruptible(const bool IsInterruptible) {
  Cxt->getCompilerConfigure().setInterruptible(IsInterruptible);
}

bool Configuration::CompilerIsInterruptible() {
  return Cxt->getCompilerConfigure().isInterruptible();
}

void Configuration::StatisticsSetInstructionCounting(const bool IsCount) {
  Cxt->getStatisticsConfigure().setInstructionCounting(IsCount);
}

bool Configuration::StatisticsIsInstructionCounting() {
  return Cxt->getStatisticsConfigure().isInstructionCounting();
}

void Configuration::StatisticsSetCostMeasuring(const bool IsMeasure) {
  Cxt->getStatisticsConfigure().setCostMeasuring(IsMeasure);
}

bool Configuration::StatisticsIsCostMeasuring() {
  return Cxt->getStatisticsConfigure().isCostMeasuring();
}

void Configuration::StatisticsSetTimeMeasuring(const bool IsMeasure) {
  Cxt->getStatisticsConfigure().setTimeMeasuring(IsMeasure);
}

bool Configuration::StatisticsIsTimeMeasuring() {
  return Cxt->getStatisticsConfigure().isTimeMeasuring();
}

// <<<<<<<< WasmEdge Configuration <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Statistics >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Statistics::Statistics() {
  this->Cxt = std::make_unique<Statistics::StatisticsContext>();
}

uint64_t Statistics::GetInstrCount() {
  return Cxt->getInstrCount();
}

double Statistics::GetInstrPerSecond() {
  return Cxt->getInstrPerSecond();
}

uint64_t Statistics::GetTotalCost() {
  return Cxt->getTotalCost();
}

void Statistics::SetCostTable(std::vector<uint64_t> &CostArr) {
  Cxt->setCostTable(CostArr);
}

void Statistics::SetCostLimit(const uint64_t Limit) {
  Cxt->setCostLimit(Limit);
}

void Statistics::Clear() {
  Cxt->clear();
}

// <<<<<<<< WasmEdge Statistics <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Runtime >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// >>>>>>>> WasmEdge Loader >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Loader::Loader(const Configuration &ConfCxt) {
  this->Cxt = std::make_unique<Loader::LoaderContext>(ConfCxt.Cxt,
                  &WasmEdge::Executor::Executor::Intrinsics);
}

Result Loader::Parse(ASTModule &Module, const std::string &Path) {
  return wrap(
      [&]() {
        return this->Cxt->parseModule(std::filesystem::absolute(Path));
      },
      [&](auto &&Res) {
        ASTModule::ASTModuleContext *Tmp =
          static_cast<ASTModule::ASTModuleContext *>((*Res).release());
        Module.Cxt.reset(Tmp);
      });
}

Result Loader::Parse(ASTModule &Module, const std::vector<uint8_t> &Buf) {
  return wrap(
      [&]() { return Cxt->parseModule(Buf); },
      [&](auto &&Res) {
        ASTModule::ASTModuleContext *Tmp =
          static_cast<ASTModule::ASTModuleContext *>((*Res).release());
        Module.Cxt.reset(Tmp);
      });
}

// <<<<<<<< WasmEdge Loader <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Validator >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Validator::Validator(Configuration &ConfCxt) {
  this->Cxt = std::make_unique<Validator::ValidatorContext>(*ConfCxt.Cxt);
}

Result Validator::Validate(const ASTModule &ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->validate(*ASTCxt.Cxt);
      },
      EmptyThen);
}

// <<<<<<<< WasmEdge Validator <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Executor >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Executor::Executor(const Configuration &ConfCxt, Statistics &StatCxt) {
  this->Cxt =
    std::make_unique<Executor::ExecutorContext>(ConfCxt.Cxt, StatCxt.Cxt.get());
}

Result Executor::Instantiate(ModuleInstance &ModuleCxt, Store &StoreCxt,
                             const ASTModule &ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->instantiateModule(*StoreCxt.Cxt, *ASTCxt.Cxt);
      },
      [&](auto &&Res) {
        ModuleInstance::ModuleInstanceContext *Tmp =
          static_cast<ModuleInstance::ModuleInstanceContext *>(
            (*Res).release());
        ModuleCxt.Cxt.reset(Tmp);
      });
}

Result Executor::Register(ModuleInstance &ModuleCxt, Store &StoreCxt,
                          const ASTModule &ASTCxt,
                          const std::string &ModuleName) {
  return wrap(
      [&]() {
        return Cxt->registerModule(*StoreCxt.Cxt, *ASTCxt.Cxt,
                                   std::string_view(ModuleName));
      },
      [&](auto &&Res) {
        auto *Tmp =
          static_cast<ModuleInstance::ModuleInstanceContext *>(
            (*Res).release());
        ModuleCxt.Cxt.reset(Tmp);
      });
}

Result Executor::Register(Store &StoreCxt,
                          const ModuleInstance &ImportCxt) {
  return wrap(
      [&]() {
        return Cxt->registerModule(*StoreCxt.Cxt, *ImportCxt.Cxt);
      },
      EmptyThen);
}

Result Executor::Invoke(const FunctionInstance &FuncCxt,
                        const std::vector<Value> &Params,
                        std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]()
          -> WasmEdge::Expect<
              std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>> {
        return Cxt->invoke(*FuncCxt.Cxt, ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); });
}

// <<<<<<<< WasmEdge Executor <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ASTModule >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

ASTModule::ASTModule() {
  this->Cxt = std::make_unique<ASTModule::ASTModuleContext>();
}

std::vector<ImportType> ASTModule::ListImports() {
  const auto &ImpSec = Cxt->getImportSection().getContent();
  std::vector<ImportType> ImpList;
  for (uint32_t I = 0; I < ImpSec.size(); I++) {
    ImpList.emplace_back(ImpSec[I]);
  }
  return ImpList;
}

std::vector<ExportType> ASTModule::ListExports() {
  const auto &ExpSec = Cxt->getExportSection().getContent();
  std::vector<ExportType> ExpList;
  for (uint32_t I = 0; I < ExpSec.size(); I++) {
    ExpList.emplace_back(ExpSec[I]);
  }
  return ExpList;
}

// <<<<<<<< WasmEdge ASTModule <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Store >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

Store::Store() {
  this->Cxt = std::make_unique<Store::StoreContext>();
}

const ModuleInstance Store::FindModule(const std::string &Name) {
  return ModuleInstance(Cxt->findModule(std::string_view(Name))); //RVO
}

std::vector<std::string> Store::ListModule() {
  std::vector<std::string> ModList;
  Cxt->getModuleList(
      [&](auto &Map) {
        for (auto &&Pair : Map) {
          ModList.emplace_back(Pair.first);
        }
      });
  return ModList;
}

// <<<<<<<< WasmEdge Store <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge ModuleInstance >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// <<<<<<<< WasmEdge ModuleInstance <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// <<<<<<<< WasmEdge Runtime <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


// >>>>>>>> WasmEdge VM >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

VM::VM(const Configuration &ConfCxt, Store &StoreCxt) {
  this->Cxt = std::make_unique<VM::VMContext>(*ConfCxt.Cxt, *StoreCxt.Cxt);
}

VM::VM(const Configuration &ConfCxt) {
  this->Cxt = std::make_unique<VM::VMContext>(*ConfCxt.Cxt);
}

VM::VM(const Store &StoreCxt) {
  this->Cxt = std::make_unique<VM::VMContext>(WasmEdge::Configure(),
      *StoreCxt.Cxt);
}

VM::VM() {
  this->Cxt = std::make_unique<VM::VMContext>(WasmEdge::Configure());
}

Result VM::RegisterModule(const std::string &ModuleName,
                          const std::string &Path) {
  return wrap(
      [&]() {
        return Cxt->registerModule(std::string_view(ModuleName),
                                  std::filesystem::absolute(Path));
      },
      EmptyThen);
}

Result VM::RegisterModule(const std::string &ModuleName,
                          const std::vector<uint8_t> &Buf) {
  return wrap(
      [&]() {
        return Cxt->registerModule(std::string_view(ModuleName),
                                   Buf);
      },
      EmptyThen);
}

Result VM::RegisterModule(const std::string &ModuleName,
                          const ASTModule &ASTCxt) {
  return wrap(
      [&]() {
        return Cxt->registerModule(std::string_view(ModuleName),
                                   *ASTCxt.Cxt);
      },
      EmptyThen);
}

Result VM::RegisterModule(const ModuleInstance &ImportCxt) {
  return wrap(
      [&]() {
        return Cxt->registerModule(*ImportCxt.Cxt);
      },
      EmptyThen);
}

Result VM::RunWasm(const std::string &Path, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->runWasmFile(std::filesystem::absolute(Path),
                                std::string_view(FuncName), ParamPair.first,
                                ParamPair.second);
      },
      [&](auto Res) { Value::ValueUtils::FillValueArr(*Res, Returns); });
}

Result VM::RunWasm(const std::vector<uint8_t> &Buf, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->runWasmFile(Buf, std::string_view(FuncName),
                                ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); });
}

Result VM::RunWasm(const ASTModule &ASTCxt, const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->runWasmFile(*ASTCxt.Cxt, std::string_view(FuncName),
                                ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); }); // TODO
}

std::unique_ptr<Async> VM::AsyncRunWasm(const std::string &Path,
              const std::string &FuncName, const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncRunWasmFile(
      std::filesystem::absolute(Path), std::string_view(FuncName),
      ParamPair.first, ParamPair.second));
}

std::unique_ptr<Async> VM::AsyncRunWasm(const std::vector<uint8_t> &Buf,
              const std::string &FuncName, const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncRunWasmFile(Buf,
      std::string_view(FuncName), ParamPair.first, ParamPair.second));
}

std::unique_ptr<Async> VM::AsyncRunWasm(const ASTModule &ASTCxt,
              const std::string &FuncName, const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncRunWasmFile(*ASTCxt.Cxt,
      std::string_view(FuncName), ParamPair.first, ParamPair.second));
}

Result VM::LoadWasm(const std::string &Path) {
  return wrap([&]() { return Cxt->loadWasm(std::filesystem::absolute(Path)); },
              EmptyThen);
}

Result VM::LoadWasm(const std::vector<uint8_t> &Buf) {
  return wrap([&]() { return Cxt->loadWasm(Buf); },
              EmptyThen);
}

Result VM::LoadWasm(const ASTModule &ASTCxt) {
  return wrap([&]() { return Cxt->loadWasm(*ASTCxt.Cxt); },
              EmptyThen);
}

Result VM::Validate() {
  return wrap([&]() { return Cxt->validate(); }, EmptyThen);
}

Result VM::Instantiate() {
  return wrap([&]() { return Cxt->instantiate(); }, EmptyThen);
}

Result VM::Execute(const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->execute(std::string_view(FuncName), ParamPair.first,
                            ParamPair.second);
      },
      [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); }); // TODO
}

Result VM::Execute(const std::string &ModuleName,
                   const std::string &FuncName,
                   const std::vector<Value> &Params,
                   std::vector<Value> &Returns) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return wrap(
      [&]() {
        return Cxt->execute(std::string_view(ModuleName),
            std::string_view(FuncName), ParamPair.first, ParamPair.second);
      },
      [&](auto &&Res) { Value::ValueUtils::FillValueArr(*Res, Returns); }); // TODO
}

std::unique_ptr<Async> VM::AsyncExecute(const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncExecute(
      std::string_view(FuncName), ParamPair.first, ParamPair.second));
}

std::unique_ptr<Async> VM::AsyncExecute(const std::string &ModuleName,
                                        const std::string &FuncName,
                                        const std::vector<Value> &Params) {
  auto ParamPair = Value::ValueUtils::GenParamPair(Params);
  return std::make_unique<Async>(Cxt->asyncExecute(
      std::string_view(ModuleName), std::string_view(FuncName),
      ParamPair.first, ParamPair.second));
}

std::unique_ptr<const FunctionType> VM::GetFunctionType(const std::string &FuncName) {
  const auto FuncList = Cxt->getFunctionList();
  for (const auto &It: FuncList) {
    if (It.first == std::string_view(FuncName)) {
      auto FuncType = std::make_unique<FunctionType>();
      FuncType->Cxt =
          std::make_unique<FunctionType::FunctionTypeContext>(It.second);
      return std::move(FuncType);
    }
  }
  return nullptr;
}

std::unique_ptr<const FunctionType> VM::GetFunctionType(const std::string &ModuleName,
                                    const std::string &FuncName) {
  const auto *ModInst =
      Cxt->getStoreManager().findModule(std::string_view(ModuleName));
  if (ModInst != nullptr) {
    const auto *FuncInst =
        ModInst->findFuncExports(std::string_view(FuncName));
    if (FuncInst != nullptr) {
      auto FuncType = std::make_unique<FunctionType>();
      FuncType->Cxt = std::make_unique<FunctionType::FunctionTypeContext>(
                                      FuncInst->getFuncType());
      return std::move(FuncType);
    }
    return nullptr;
  }
  return nullptr;
}

void VM::Cleanup() {
  Cxt->cleanup();
}

uint32_t VM::GetFunctionList(std::vector<std::string> &Names,
                             std::vector<const FunctionType> &FuncTypes) {
  // Not to use VM::getFunctionList() here because not to allocate the
  // returned function name strings.
  const auto *ModInst = Cxt->getActiveModule();
  if (ModInst != nullptr) {
    return ModInst->getFuncExports([&](const auto &FuncExp) {
      for (auto It = FuncExp.cbegin(); It != FuncExp.cend(); It++) {
        const auto *FuncInst = It->second;
        const auto &FuncTypeCxt = FuncInst->getFuncType();

        auto FuncType = std::make_unique<FunctionType>();
        FuncType->Cxt =
            std::make_unique<FunctionType::FunctionTypeContext>(FuncTypeCxt);
        FuncTypes.push_back(std::move(*FuncType));
      }
      return static_cast<uint32_t>(FuncExp.size());
    });
  }
  return 0;
}

ModuleInstance &VM::GetImportModuleContext(const HostRegistration Reg) {
  auto Ptr = Cxt->getImportModule(static_cast<WasmEdge::HostRegistration>(Reg));
}

const ModuleInstance &VM::GetActiveModule() {

}

const ModuleInstance &VM::GetRegisteredModule(const std::string &ModuleName) {

}

std::vector<std::string> VM::ListRegisteredModule() {

}

Store &VM::GetStoreContext() {

}

Loader &VM::GetLoaderContext() {

}

Validator &VM::GetValidatorContext() {

}

Executor &VM::GetExecutorContext() {

}

Statistics &GetStatisticsContext() {

}
// <<<<<<<< WasmEdge VM <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}
}
