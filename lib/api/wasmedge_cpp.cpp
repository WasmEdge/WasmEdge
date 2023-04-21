#include "wasmedge/wasmedge.hh"

#include "driver/compiler.h"
#include "driver/tool.h"
#include "vm/vm.h"

namespace WasmEdge {

// >>>>>>>> WasmEdge ASTModule members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class ASTModule::ASTModuleContext: public AST::Module {};

// <<<<<<<< WasmEdge ASTModule members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

bool Limit::operator==(const Limit &Lim) {
  return this->HasMax == Lim.HasMax && this->Shared == Lim.Shared &&
         this->Min == Lim.Min && this->Max == Lim.Max;
}

// >>>>>>>> WasmEdge FunctionType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class FunctionType::FunctionTypeContext: public AST::FunctionType {
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
  return Cxt->getParamTypes();
}

const std::vector<ValType> &FunctionType::GetReturns() {
  return Cxt->getReturnTypes();
}

// <<<<<<<< WasmEdge FunctionType members <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge TableType members >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class TableType::TableTypeContext: public AST::TableType {
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

class MemoryType::MemoryTypeContext: public AST::MemoryType {
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

class GlobalType::GlobalTypeContext: public AST::GlobalType {
public:
  GlobalTypeContext(AST::GlobalType &GlobType): AST::GlobalType(GlobType) {}
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

class ImportType::ImportTypeContext: public AST::ImportDesc {};

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

class ExportType::ExportTypeContext: public AST::ExportDesc {};

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

}
