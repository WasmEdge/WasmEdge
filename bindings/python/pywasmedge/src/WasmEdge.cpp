#include "WasmEdge.hpp"

std::string pysdk::logging::_str = "logging: Level not set";

/* --------------- Python Module ----------------------------------------*/

PYBIND11_MODULE(WasmEdge, module) {

  module.def("version", WasmEdge_VersionGet);

  pybind11::class_<pysdk::logging>(module, "Logging")
      .def(pybind11::init())
      .def("__doc__", &pysdk::logging::doc)
      .def("__str__", &pysdk::logging::str)
      .def_static("error", &pysdk::logging::error)
      .def_static("debug", &pysdk::logging::debug);

  /*Overloading Python add and remove functions for Configure class*/

  void (pysdk::Configure::*add_prop)(WasmEdge_Proposal) =
      &pysdk::Configure::add;
  void (pysdk::Configure::*remove_prop)(WasmEdge_Proposal) =
      &pysdk::Configure::remove;
  void (pysdk::Configure::*add_host)(WasmEdge_HostRegistration) =
      &pysdk::Configure::add;
  void (pysdk::Configure::*remove_host)(WasmEdge_HostRegistration) =
      &pysdk::Configure::remove;

  pybind11::class_<pysdk::Configure>(module, "Configure")
      .def(pybind11::init())
      .def("__doc__", &pysdk::Configure::doc)
      .def("add", add_prop)
      .def("remove", remove_prop)
      .def("add", add_host)
      .def("remove", remove_host)
      .def_property("max_paging", &pysdk::Configure::get_max_paging,
                    &pysdk::Configure::set_max_paging)
      .def_property("optimization_level", &pysdk::Configure::get_opt_level,
                    &pysdk::Configure::set_opt_level);

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

  /* TODO: Find suitable use for WasmEdge WASM value struct from python
   * perspective */
  pybind11::class_<pysdk::Value>(module, "Value")
      .def(pybind11::init<pybind11::object>())
      .def_property("Value", &pysdk::Value::get_value, &pysdk::Value::set_value)
      .def_property_readonly("Type", &pysdk::Value::get_type);

  pybind11::class_<pysdk::result>(module, "Result")
      .def(pybind11::init())
      .def("__doc__", &pysdk::result::doc)
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
      .def("__doc__", &pysdk::Store::doc)
      .def("listFunctions", &pysdk::Store::listFunctions)
      .def("listModules", &pysdk::Store::listModules)
      .def("listRegisteredFunctions", &pysdk::Store::listRegisteredFunctions);

  pybind11::class_<pysdk::ASTModuleCxt>(module, "ASTModule")
      .def(pybind11::init())
      .def("imports", &pysdk::ASTModuleCxt::listImports)
      .def("exports", &pysdk::ASTModuleCxt::listExports);

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

  /*Overloading VM run functions*/

  pybind11::tuple (pysdk::VM::*run_step_by_step)(
      pybind11::object, pybind11::object, pybind11::object) = &pysdk::VM::run;
  pybind11::tuple (pysdk::VM::*run)(pybind11::object, pybind11::object,
                                    pybind11::object, pybind11::object,
                                    pybind11::object) = &pysdk::VM::run;
  pybind11::tuple (pysdk::VM::*run_wasm_buffer)(
      pybind11::object, pybind11::object, pybind11::object, std::string &) =
      &pysdk::VM::run;

  pybind11::class_<pysdk::VM>(module, "VM")
      .def(pybind11::init())
      .def(pybind11::init<pysdk::Configure &>())
      .def(pybind11::init<pysdk::Store &>())
      .def(pybind11::init<pysdk::Configure &, pysdk::Store &>())
      .def("__doc__", &pysdk::VM::doc)
      .def("run", run)
      .def("run", run_step_by_step)
      .def("run", run_wasm_buffer)
      .def("register", &pysdk::VM::register_module_from_ast)
      .def("register", &pysdk::VM::register_module_from_buffer)
      .def("register", &pysdk::VM::register_module_from_file)
      .def("register", &pysdk::VM::register_module_from_import_object)
      .def("ListExportedFunctions", &pysdk::VM::list_exported_functions);

  pybind11::class_<pysdk::function>(module, "Function")
      .def(pybind11::init<pybind11::function>());

  pybind11::class_<pysdk::import_object>(module, "ImportObject")
      .def(pybind11::init<std::string>())
      .def("add", &pysdk::import_object::add);
};

/* --------------- Python Module End ----------------------------------------*/