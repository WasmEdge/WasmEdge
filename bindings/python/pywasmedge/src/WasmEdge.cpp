#include "WasmEdge.hpp"
#include "doc_strings.hpp"

std::string pysdk::logging::_str = "logging: Level not set";

/* --------------- Python Module ----------------------------------------*/

PYBIND11_MODULE(WasmEdge, module) {
  module.doc() = pysdk::doc::module;

  module.def("VersionGet", WasmEdge_VersionGet, pysdk::doc::VersionGet);
  module.def("VersionGetMajor", WasmEdge_VersionGetMajor,
             pysdk::doc::VersionGetMajor);
  module.def("VersionGetMinor", WasmEdge_VersionGetMinor,
             pysdk::doc::VersionGetMinor);
  module.def("VersionGetPatch", WasmEdge_VersionGetPatch,
             pysdk::doc::VersionGetPatch);

  pybind11::class_<pysdk::logging>(module, "Logging", pysdk::doc::Logging)
      .def(pybind11::init())
      .def("__str__", &pysdk::logging::str)
      .def_static("error", &pysdk::logging::error, pysdk::doc::error)
      .def_static("debug", &pysdk::logging::debug, pysdk::doc::debug);

  pybind11::class_<pysdk::Configure>(module, "Configure", pysdk::doc::Configure)
      .def(pybind11::init())
      .def("AddProposal", &pysdk::Configure::AddProposal)
      .def("AddHostRegistration", &pysdk::Configure::AddHostRegistration)
      .def("RemoveProposal", &pysdk::Configure::RemoveProposal)
      .def("RemoveHostRegistration", &pysdk::Configure::RemoveHostRegistration)
      .def("CompilerGetOptimizationLevel",
           &pysdk::Configure::CompilerGetOptimizationLevel)
      .def("CompilerGetOutputFormat",
           &pysdk::Configure::CompilerGetOutputFormat)
      .def("CompilerIsDumpIR", &pysdk::Configure::CompilerIsDumpIR)
      .def("CompilerIsGenericBinary",
           &pysdk::Configure::CompilerIsGenericBinary)
      .def("CompilerIsInterruptible",
           &pysdk::Configure::CompilerIsInterruptible)
      .def("CompilerSetDumpIR", &pysdk::Configure::CompilerSetDumpIR)
      .def("CompilerSetGenericBinary",
           &pysdk::Configure::CompilerSetGenericBinary)
      .def("CompilerSetInterruptible",
           &pysdk::Configure::CompilerSetInterruptible)
      .def("CompilerSetOptimizationLevel",
           &pysdk::Configure::CompilerSetOptimizationLevel)
      .def("CompilerSetOutputFormat",
           &pysdk::Configure::CompilerSetOutputFormat)
      .def("GetMaxMemoryPage", &pysdk::Configure::GetMaxMemoryPage)
      .def("SetMaxMemoryPage", &pysdk::Configure::SetMaxMemoryPage)
      .def("HasHostRegistration", &pysdk::Configure::HasHostRegistration)
      .def("HasProposal", &pysdk::Configure::HasProposal)
      .def("StatisticsIsCostMeasuring",
           &pysdk::Configure::StatisticsIsCostMeasuring)
      .def("StatisticsIsInstructionCounting",
           &pysdk::Configure::StatisticsIsInstructionCounting)
      .def("StatisticsIsTimeMeasuring",
           &pysdk::Configure::StatisticsIsTimeMeasuring)
      .def("StatisticsSetCostMeasuring",
           &pysdk::Configure::StatisticsSetCostMeasuring)
      .def("StatisticsSetInstructionCounting",
           &pysdk::Configure::StatisticsSetInstructionCounting)
      .def("StatisticsSetTimeMeasuring",
           &pysdk::Configure::StatisticsSetTimeMeasuring);

  pybind11::class_<pysdk::Statistics>(module, "Statistics")
      .def(pybind11::init())
      .def("GetInstrCount", &pysdk::Statistics::GetInstrCount)
      .def("GetInstrPerSecond", &pysdk::Statistics::GetInstrPerSecond)
      .def("GetTotalCost", &pysdk::Statistics::GetTotalCost)
      .def("SetCostLimit", &pysdk::Statistics::SetCostLimit)
      .def("SetCostTable", &pysdk::Statistics::SetCostTable);

  pybind11::enum_<WasmEdge_CompilerOptimizationLevel>(module, "Optimization")
      .value("O0", WasmEdge_CompilerOptimizationLevel_O0)
      .value("O1", WasmEdge_CompilerOptimizationLevel_O1)
      .value("O2", WasmEdge_CompilerOptimizationLevel_O2)
      .value("O3", WasmEdge_CompilerOptimizationLevel_O3)
      .value("Os", WasmEdge_CompilerOptimizationLevel_Os)
      .value("Oz", WasmEdge_CompilerOptimizationLevel_Oz)
      .export_values();

  pybind11::enum_<WasmEdge_CompilerOutputFormat>(module, "CompilerOutput")
      .value("Native", WasmEdge_CompilerOutputFormat_Native)
      .value("Wasm", WasmEdge_CompilerOutputFormat_Wasm)
      .export_values();

  /* WasmEdge WASM value struct. */
  pybind11::enum_<WasmEdge_ValType>(module, "Type")
      .value("I32", WasmEdge_ValType_I32)
      .value("I64", WasmEdge_ValType_I64)
      .value("F32", WasmEdge_ValType_F32)
      .value("F64", WasmEdge_ValType_F64)
      .value("V128", WasmEdge_ValType_V128)
      .value("FuncRef", WasmEdge_ValType_FuncRef)
      .value("ExternRef", WasmEdge_ValType_ExternRef)
      .export_values();

  pybind11::enum_<WasmEdge_ExternalType>(module, "ExternalType")
      .value("Func", WasmEdge_ExternalType_Function)
      .value("Glob", WasmEdge_ExternalType_Global)
      .value("Mem", WasmEdge_ExternalType_Memory)
      .value("Tab", WasmEdge_ExternalType_Table)
      .export_values();

  pybind11::enum_<WasmEdge_RefType>(module, "RefType")
      .value("FuncRef", WasmEdge_RefType_FuncRef)
      .value("ExternRef", WasmEdge_RefType_ExternRef)
      .export_values();

  /* TODO: Find suitable use for WasmEdge WASM value struct from python
   * perspective */
  pybind11::class_<pysdk::Value>(module, "Value")
      .def(pybind11::init<pybind11::object, WasmEdge_ValType &>())
      .def_property("Value", &pysdk::Value::get_value, &pysdk::Value::set_value)
      .def_property_readonly("Type", &pysdk::Value::get_type);

  pybind11::class_<pysdk::result>(module, "Result")
      .def(pybind11::init())
      .def(pybind11::init<int &>())
      .def("__str__", &pysdk::result::message)
      .def("__bool__", &pysdk::result::operator bool)
      .def("message", &pysdk::result::message)
      .def("code", &pysdk::result::get_code);

  pybind11::enum_<WasmEdge_Proposal>(module, "Proposal")
      .value("ImportExportMutGlobals", WasmEdge_Proposal_ImportExportMutGlobals)
      .value("NonTrapFloatToIntConversions",
             WasmEdge_Proposal_NonTrapFloatToIntConversions)
      .value("BulkMemoryOperations", WasmEdge_Proposal_BulkMemoryOperations)
      .value("ReferenceTypes", WasmEdge_Proposal_ReferenceTypes)
      .value("SIMD", WasmEdge_Proposal_SIMD)
      .value("TailCall", WasmEdge_Proposal_TailCall)
      .value("Annotations", WasmEdge_Proposal_Annotations)
      .value("Memory64", WasmEdge_Proposal_Memory64)
      .value("Threads", WasmEdge_Proposal_Threads)
      .value("ExceptionHandling", WasmEdge_Proposal_ExceptionHandling)
      .value("FunctionReferences", WasmEdge_Proposal_FunctionReferences)
      .export_values();

  pybind11::enum_<WasmEdge_HostRegistration>(module, "Host")
      .value("Wasi", WasmEdge_HostRegistration_Wasi)
      .value("WasmEdge", WasmEdge_HostRegistration_WasmEdge_Process)
      .export_values();

  pybind11::class_<pysdk::Store>(module, "Store")
      .def(pybind11::init())
      .def("FindMemory", &pysdk::Store::FindMemory)
      .def("ListFunction", &pysdk::Store::ListFunction)
      .def("ListFunctionRegistered", &pysdk::Store::ListFunctionRegistered)
      .def("FindFunction", &pysdk::Store::FindFunction)
      .def("FindFunctionRegistered", &pysdk::Store::FindFunctionRegistered)
      .def("FindGlobal", &pysdk::Store::FindGlobal)
      .def("FindGlobalRegistered", &pysdk::Store::FindGlobalRegistered)
      .def("FindMemoryRegistered", &pysdk::Store::FindMemoryRegistered)
      .def("FindTable", &pysdk::Store::FindTable)
      .def("FindTableRegistered", &pysdk::Store::FindTableRegistered)
      .def("ListFunctionLength", &pysdk::Store::ListFunctionLength)
      .def("ListFunctionRegisteredLength",
           &pysdk::Store::ListFunctionRegisteredLength)
      .def("ListGlobal", &pysdk::Store::ListGlobal)
      .def("ListGlobalRegistered", &pysdk::Store::ListGlobalRegistered)
      .def("ListGlobalLength", &pysdk::Store::ListGlobalLength)
      .def("ListGlobalRegisteredLength",
           &pysdk::Store::ListGlobalRegisteredLength)
      .def("ListMemory", &pysdk::Store::ListMemory)
      .def("ListMemoryLength", &pysdk::Store::ListMemoryLength)
      .def("ListMemoryRegistered", &pysdk::Store::ListMemoryRegistered)
      .def("ListMemoryRegisteredLength",
           &pysdk::Store::ListMemoryRegisteredLength)
      .def("ListModule", &pysdk::Store::ListModule)
      .def("ListModuleLength", &pysdk::Store::ListModuleLength)
      .def("ListTable", &pysdk::Store::ListTable)
      .def("ListTableLength", &pysdk::Store::ListTableLength)
      .def("ListTableRegistered", &pysdk::Store::ListTableRegistered)
      .def("ListTableRegisteredLength",
           &pysdk::Store::ListTableRegisteredLength);

  pybind11::class_<pysdk::ExportType>(module, "ExportType")
      .def(pybind11::init())
      .def("GetExternalName", &pysdk::ExportType::GetExternalName)
      .def("GetExternalType", &pysdk::ExportType::GetExternalType)
      .def("GetFunctionType", &pysdk::ExportType::GetFunctionType)
      .def("GetGlobalType", &pysdk::ExportType::GetGlobalType)
      .def("GetMemoryType", &pysdk::ExportType::GetMemoryType);

  pybind11::class_<pysdk::ASTModuleCxt>(module, "ASTModule")
      .def(pybind11::init())
      .def("ListExports", &pysdk::ASTModuleCxt::ListExports)
      .def("ListExportsLength", &pysdk::ASTModuleCxt::ListExportsLength)
      .def("ListImports", &pysdk::ASTModuleCxt::ListImports)
      .def("ListImportsLength", &pysdk::ASTModuleCxt::ListImportsLength);

  /* Loader parse function overload */
  auto (pysdk::Loader::*parse_file)(pysdk::ASTModuleCxt &, std::string &) =
      &pysdk::Loader::parse;
  auto (pysdk::Loader::*parse_bytes)(pysdk::ASTModuleCxt &, pybind11::tuple) =
      &pysdk::Loader::parse;

  pybind11::class_<pysdk::Loader>(module, "Loader")
      .def(pybind11::init<pysdk::Configure &>())
      .def("parse", parse_file)
      .def("parse", parse_bytes);

  pybind11::class_<pysdk::Validator>(module, "Validator")
      .def(pybind11::init<pysdk::Configure &>())
      .def("validate", &pysdk::Validator::validate);

  pybind11::class_<pysdk::Executor>(module, "Executor")
      .def(pybind11::init<pysdk::Configure &>())
      .def("instantiate", &pysdk::Executor::instantiate)
      .def("invoke", &pysdk::Executor::invoke);

  pybind11::class_<pysdk::VM>(module, "VM")
      .def(pybind11::init())
      .def(pybind11::init<pysdk::Configure &>())
      .def(pybind11::init<pysdk::Store &>())
      .def(pybind11::init<pysdk::Configure &, pysdk::Store &>())
      .def("RunWasmFromFile", &pysdk::VM::run_from_wasm_file)
      .def("RunWasmFromBuffer", &pysdk::VM::run_from_buffer)
      .def("RunWasmFromASTModule", &pysdk::VM::run_from_ast)
      .def("RegisterModuleFromASTModule", &pysdk::VM::register_module_from_ast)
      .def("RegisterModuleFromBuffer", &pysdk::VM::register_module_from_buffer)
      .def("RegisterModuleFromFile", &pysdk::VM::register_module_from_file)
      .def("RegisterModuleFromImport",
           &pysdk::VM::register_module_from_import_object)
      .def("GetFunctionList", &pysdk::VM::get_functions)
      .def("GetFunctionListLength", &pysdk::VM::get_functions_len)
      .def("GetFunctionType", &pysdk::VM::get_function_type)
      .def("GetFunctionTypeRegistered",
           &pysdk::VM::get_function_type_registered)
      .def("GetImportModuleContext", &pysdk::VM::get_import_module_context)
      .def("GetStatistics", &pysdk::VM::get_statistics_context)
      .def("GetStoreContext", &pysdk::VM::get_store_cxt)
      .def("Instantiate", &pysdk::VM::instantiate)
      .def("LoadWasmFromASTModule", &pysdk::VM::load_from_ast)
      .def("LoadWasmFromBuffer", &pysdk::VM::load_from_buffer)
      .def("LoadWasmFromFile", &pysdk::VM::load_from_file)
      .def("ExecuteRegistered", &pysdk::VM::execute_registered)
      .def("Execute", &pysdk::VM::execute)
      .def("Validate", &pysdk::VM::validate);

  pybind11::class_<pysdk::FunctionTypeContext>(module, "FunctionType")
      .def(pybind11::init<pybind11::list, pybind11::list>())
      .def("GetParamLen", &pysdk::FunctionTypeContext::get_param_len)
      .def("GetParamTypes", &pysdk::FunctionTypeContext::get_param_types)
      .def("GetRetLen", &pysdk::FunctionTypeContext::get_ret_len)
      .def("GetRetTypes", &pysdk::FunctionTypeContext::get_ret_types);

  pybind11::class_<pysdk::Function>(module, "Function")
      .def(pybind11::init<pysdk::FunctionTypeContext &, pybind11::function,
                          uint64_t &>())
      .def("GetType", &pysdk::Function::get_func_type);

  pybind11::class_<pysdk::ImportTypeContext>(module, "ImportType")
      .def(pybind11::init<>())
      .def("GetFunctionType", &pysdk::ImportTypeContext::get_function_type_cxt)
      .def("GetExternalType", &pysdk::ImportTypeContext::get_external_type)
      .def("GetGlobalType", &pysdk::ImportTypeContext::get_global_type_cxt)
      .def("GetExternalName", &pysdk::ImportTypeContext::get_external_name);

  pybind11::class_<pysdk::import_object>(module, "ImportObject")
      .def(pybind11::init<std::string &>())
      .def("AddFunction", &pysdk::import_object::add);

  pybind11::class_<WasmEdge_Limit>(module, "Limit")
      .def(pybind11::init<bool, uint32_t, uint32_t>())
      .def_property_readonly("HasMax",
                             [](const WasmEdge_Limit &L) { return L.HasMax; })
      .def_property_readonly("Max",
                             [](const WasmEdge_Limit &L) { return L.Max; })
      .def_property_readonly("Min",
                             [](const WasmEdge_Limit &L) { return L.Min; });

  pybind11::class_<pysdk::MemoryTypeCxt>(module, "MemoryType")
      .def(pybind11::init<WasmEdge_Limit &>())
      .def("GetLimit", [](pysdk::MemoryTypeCxt &m) {
        return WasmEdge_MemoryTypeGetLimit(m.get());
      });

  pybind11::enum_<WasmEdge_Mutability>(module, "Mutability")
      .value("Var", WasmEdge_Mutability_Var)
      .value("Const", WasmEdge_Mutability_Const)
      .export_values();

  pybind11::class_<pysdk::GlobalTypeCxt>(module, "GlobalType")
      .def(pybind11::init<const WasmEdge_ValType &,
                          const WasmEdge_Mutability &>())
      .def("GetMutability",
           [](pysdk::GlobalTypeCxt &g) {
             return WasmEdge_GlobalTypeGetMutability(g.get());
           })
      .def("GetValType", [](pysdk::GlobalTypeCxt &g) {
        return WasmEdge_GlobalTypeGetValType(g.get());
      });

  pybind11::class_<pysdk::Global>(module, "Global")
      .def(pybind11::init())
      .def("GetGlobalType", &pysdk::Global::GetGlobalType)
      .def("GetValue", &pysdk::Global::GetValue)
      .def("SetValue", &pysdk::Global::SetValue);

  pybind11::class_<pysdk::Memory>(module, "Memory")
      .def(pybind11::init<pysdk::MemoryTypeCxt &>())
      .def("SetData", &pysdk::Memory::set_data)
      .def("GetData", &pysdk::Memory::get_data)
      .def("GetPageSize", &pysdk::Memory::get_page_size)
      .def("GrowPage", &pysdk::Memory::grow_page)
      .def("GetMemoryType", &pysdk::Memory::get_type);

  pybind11::class_<pysdk::TableTypeCxt>(module, "TableType")
      .def(pybind11::init<WasmEdge_RefType &, WasmEdge_Limit &>())
      .def("GetLimit",
           [](pysdk::TableTypeCxt &tcxt) {
             return WasmEdge_TableTypeGetLimit(tcxt.get());
           })
      .def("GetRefType", [](pysdk::TableTypeCxt &tcxt) {
        return WasmEdge_TableTypeGetRefType(tcxt.get());
      });

  pybind11::class_<pysdk::Table>(module, "Table")
      .def(pybind11::init<pysdk::TableTypeCxt &>())
      .def("GetType", &pysdk::Table::get_type)
      .def("GetSize", &pysdk::Table::get_size)
      .def("GrowSize", &pysdk::Table::grow_size)
      .def("SetData", &pysdk::Table::set_data)
      .def("GetData", &pysdk::Table::get_data);
};

/* --------------- Python Module End ----------------------------------------*/