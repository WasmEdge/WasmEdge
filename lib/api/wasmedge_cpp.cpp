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
}

namespace WasmEdge {
namespace SDK {

// >>>>>>>> WasmEdge Version members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

Value::Value(const RefType Val)
: Val(to_uint128_t(WasmEdge::UnknownRef())),
  Type(static_cast<ValType>(Val)) {}

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

// >>>>>>>> WasmEdge ASTModule members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class ASTModule::ASTModuleContext: public WasmEdge::AST::Module {};

// <<<<<<<< WasmEdge ASTModule members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool Limit::operator==(const Limit &Lim) {
  return this->HasMax == Lim.HasMax && this->Shared == Lim.Shared &&
         this->Min == Lim.Min && this->Max == Lim.Max;
}

// >>>>>>>> WasmEdge FunctionType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class FunctionType::FunctionTypeContext: public WasmEdge::AST::FunctionType {
public:
  FunctionTypeContext(AST::FunctionType &Func)
  : AST::FunctionType(Func) {}
};

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

const std::vector<ValType> &FunctionType::GetParameters() {
  auto OutputParams = std::make_unique<std::vector<ValType>>();
  auto FuncParams = Cxt->getParamTypes();
  OutputParams->resize(FuncParams.size());
  std::transform(FuncParams.begin(), FuncParams.end(),
                 OutputParams->begin(),
                 [](auto ValT) {return static_cast<ValType>(ValT)});
  return *OutputParams;
}

const std::vector<ValType> &FunctionType::GetReturns() {
  auto OutputReturns = std::make_unique<std::vector<ValType>>();
  auto FuncReturns = Cxt->getReturnTypes();
  OutputReturns->resize(FuncReturns.size());
  std::transform(FuncReturns.begin(), FuncReturns.end(),
                 OutputReturns->begin(),
                 [](auto ValT) {return static_cast<ValType>(ValT)});
  return *OutputReturns;
}

// <<<<<<<< WasmEdge FunctionType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge TableType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class TableType::TableTypeContext: public WasmEdge::AST::TableType {
public:
  TableTypeContext(AST::TableType &TabType): AST::TableType(TabType) {}
};

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
  return *TableLim;
}

// <<<<<<<< WasmEdge TableType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge MemoryType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class MemoryType::MemoryTypeContext: public WasmEdge::AST::MemoryType {
public:
  MemoryTypeContext(AST::MemoryType &MemType): AST::MemoryType(MemType) {}
};

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
  return *MemoryLim;
}

// <<<<<<<< WasmEdge MemoryType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge GlobalType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class GlobalType::GlobalTypeContext: public WasmEdge::AST::GlobalType {
public:
  GlobalTypeContext(AST::GlobalType &GlobType)
  : WasmEdge::AST::GlobalType(GlobType) {}
};

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

class ImportType::ImportTypeContext: public WasmEdge::AST::ImportDesc {};

ImportType::ImportType() {
  this->Cxt = std::make_unique<ImportType::ImportTypeContext>();
}

ExternalType ImportType::GetExternalType() {
  return static_cast<ExternalType>(Cxt->getExternalType());
}

std::string ImportType::GetModuleName() {
  std::string str{ Cxt->getModuleName() };
  return str;
}

std::string ImportType::GetExternalName() {
  std::string str{ Cxt->getExternalName() };
  return str;
}

std::shared_ptr<const FunctionType>
ImportType::GetFunctionType(const ASTModule &ASTCxt) {
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Function) {
    uint32_t Idx = Cxt->getExternalFuncTypeIdx();
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
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Table) {
    auto TabTypeCxt =
        std::make_unique<TableType::TableTypeContext>(
          Cxt->getExternalTableType());
    auto TabType = std::shared_ptr<TableType>();
    TabType->Cxt = std::move(TabTypeCxt);
    return TabType;
  }
  return nullptr;
}

std::shared_ptr<const MemoryType>
ImportType::GetMemoryType(const ASTModule &ASTCxt) {
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Memory) {
    auto ExternalMemType = Cxt->getExternalMemoryType();
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
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Global) {
    auto ExternalGlobType = Cxt->getExternalGlobalType();
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

class ExportType::ExportTypeContext: public WasmEdge::AST::ExportDesc {};

ExportType::ExportType() {
  this->Cxt = std::make_unique<ExportType::ExportTypeContext>();
}

ExternalType ExportType::GetExternalType() {
  return static_cast<ExternalType>(Cxt->getExternalType());
}

std::string ExportType::GetExternalName() {
  std::string str{ Cxt->getExternalName() };
  return str;
}

std::shared_ptr<const FunctionType>
ExportType::GetFunctionType (const ASTModule &ASTCxt) {
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Function) {
    // `external_index` = `func_index` + `import_func_nums`
    uint32_t ExtIdx = Cxt->getExternalIndex();
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
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Table) {
    // `external_index` = `table_type_index` + `import_table_nums`
    uint32_t ExtIdx = Cxt->getExternalIndex();
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
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Memory) {
    // `external_index` = `memory_type_index` + `import_memory_nums`
    uint32_t ExtIdx = Cxt->getExternalIndex();
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
  if (Cxt->getExternalType() == WasmEdge::ExternalType::Global) {
    // `external_index` = `global_type_index` + `import_global_nums`
    uint32_t ExtIdx = Cxt->getExternalIndex();
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

class Async::AsyncContext: public WasmEdge::VM::Async<
  WasmEdge::Expect<
      std::vector<std::pair<WasmEdge::ValVariant, WasmEdge::ValType>>>> {
  template <typename... Args>
  AsyncContext(Args &&...Vals)
  : VM::Async(std::forward<Args>(Vals)...) {}
};

Async::Async() {
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

}
}
