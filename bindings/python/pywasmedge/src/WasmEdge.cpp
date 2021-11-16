#include "WasmEdge.hpp"

/* --------------- VM -------------------------------- */
pysdk::VM::VM() { VMCxt = WasmEdge_VMCreate(NULL, NULL); }

pysdk::VM::VM(pysdk::Store &store) {
  VMCxt = WasmEdge_VMCreate(NULL, store.get());
}

pysdk::VM::VM(pysdk::Configure &cfg) {
  VMCxt = WasmEdge_VMCreate(cfg.get(), NULL);
}

pysdk::VM::VM(pysdk::Configure &cfg, pysdk::Store &store) {
  VMCxt = WasmEdge_VMCreate(cfg.get(), store.get());
}

pysdk::VM::~VM() { WasmEdge_VMDelete(VMCxt); }

/**
 * @brief Execute VM
 *
 * @param _FileName Name of .wasm binary
 * @param _FuncName Wasm Function name
 * @param _params Wasm Function Parameters
 * @param _param_types List of `Type` of parameters
 * @param _ret_types List of `Type` of Return values. This length must be same
 * as return length of wasm function.
 * @return pybind11::tuple
 */
pybind11::tuple pysdk::VM::run(pybind11::object _FileName,
                               pybind11::object _FuncName,
                               pybind11::object _params,
                               pybind11::object _param_types,
                               pybind11::object _ret_types) {
  std::string FileName = _FileName.cast<std::string>();
  std::string FuncName = _FuncName.cast<std::string>();
  const int ret_len = pybind11::len(_ret_types);

  auto param_len = pybind11::len(_params);
  auto param_list = _params.cast<pybind11::list>();
  auto _param_types_list = _param_types.cast<pybind11::list>();
  auto _ret_types_list = _ret_types.cast<pybind11::list>();

  WasmEdge_Value Params[param_len];
  WasmEdge_Value Returns[ret_len];
  for (int i = 0; i < param_len; i++) {
    switch (_param_types_list[i].cast<WasmEdge_ValType>()) {
    case WasmEdge_ValType_I32:
      Params[i] = WasmEdge_ValueGenI32(param_list[i].cast<int32_t>());
      break;
    case WasmEdge_ValType_I64:
      Params[i] = WasmEdge_ValueGenI64(param_list[i].cast<int64_t>());
      break;
    case WasmEdge_ValType_F32:
      Params[i] = WasmEdge_ValueGenF32(param_list[i].cast<float>());
      break;
    case WasmEdge_ValType_F64:
      Params[i] = WasmEdge_ValueGenF32(param_list[i].cast<double>());
      break;
    case WasmEdge_ValType_V128:
      Params[i] = WasmEdge_ValueGenV128(param_list[i].cast<int128_t>());
      break;
    case WasmEdge_ValType_FuncRef:
      Params[i] = WasmEdge_ValueGenFuncRef(param_list[i].cast<uint32_t>());
      break;
    // TODO: Handle Pointer
    // case WasmEdge_ValType_ExternRef:
    //   Params[i] = WasmEdge_ValueGenExternRef(
    //       param_list[i].cast<(void *)>());
    //   break;
    default:
      break;
    }
  }

  WasmEdge_String funcName{(uint32_t)FuncName.length(), FuncName.c_str()};
  pysdk::result res(WasmEdge_VMRunWasmFromFile(
      VMCxt, FileName.c_str(), funcName, Params, param_len, Returns, ret_len));

  pybind11::list returns;
  for (int i = 0; i < ret_len; i++) {
    switch (_ret_types_list[i].cast<WasmEdge_ValType>()) {
    case WasmEdge_ValType_I32:
      returns.append(pybind11::cast(WasmEdge_ValueGetI32(Returns[i])));
      break;
    case WasmEdge_ValType_I64:
      returns.append(pybind11::cast(WasmEdge_ValueGetI64(Returns[i])));
      break;
    case WasmEdge_ValType_F32:
      returns.append(pybind11::cast(WasmEdge_ValueGetF32(Returns[i])));
      break;
    case WasmEdge_ValType_F64:
      returns.append(pybind11::cast(WasmEdge_ValueGetF64(Returns[i])));
      break;
    case WasmEdge_ValType_V128:
      returns.append(pybind11::cast(WasmEdge_ValueGetV128(Returns[i])));
      break;
    case WasmEdge_ValType_FuncRef:
      returns.append(pybind11::cast(WasmEdge_ValueGetFuncIdx(Returns[i])));
      break;
    // TODO: Handle Void Pointer
    // case WasmEdge_ValType_ExternRef:
    //   returns.append(pybind11::cast(WasmEdge_ValueGetExternRef(Returns[i])));
    //   break;
    default:
      break;
    }
  }
  return pybind11::make_tuple(res, returns);
}

/**
 * @brief Execute VM without specifying parameter, return types and length.
 * Executes VM step by step.
 *
 * @param _FileName Name of .wasm binary
 * @param _FuncName Wasm Function name
 * @param _params Wasm Function Parameters
 * @return pybind11::tuple
 */
pybind11::tuple pysdk::VM::run(pybind11::object _FileName,
                               pybind11::object _FuncName,
                               pybind11::object _params) {
  pybind11::list returns;

  std::string FileName = _FileName.cast<std::string>();
  std::string FuncName = _FuncName.cast<std::string>();

  pysdk::result res(WasmEdge_VMLoadWasmFromFile(VMCxt, FileName.c_str()));
  if (!res) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(res, NULL);
  }

  res = WasmEdge_VMValidate(VMCxt);
  if (!res) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(res, NULL);
  }

  res = WasmEdge_VMInstantiate(VMCxt);
  if (!res) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(res, NULL);
  }

  WasmEdge_FunctionTypeContext *FuncTypeCxt =
      (WasmEdge_FunctionTypeContext *)WasmEdge_VMGetFunctionType(
          VMCxt, WasmEdge_StringCreateByCString(FuncName.c_str()));

  auto params_list = _params.cast<pybind11::list>();
  auto param_len = pybind11::len(_params);
  auto param_len_api = WasmEdge_FunctionTypeGetParametersLength(FuncTypeCxt);

  if (param_len != param_len_api) {
    /* TODO: Handle errors gracefully */
    WasmEdge_FunctionTypeDelete(FuncTypeCxt);
    return pybind11::make_tuple(NULL, NULL);
  }

  WasmEdge_ValType val_type_list_param[param_len];
  WasmEdge_FunctionTypeGetParameters(FuncTypeCxt, val_type_list_param,
                                     param_len);
  WasmEdge_Value Params[param_len];
  for (int i = 0; i < param_len; i++) {
    switch (val_type_list_param[i]) {
    case WasmEdge_ValType_I32:
      Params[i] = WasmEdge_ValueGenI32(params_list[i].cast<int32_t>());
      break;
    case WasmEdge_ValType_I64:
      Params[i] = WasmEdge_ValueGenI64(params_list[i].cast<int64_t>());
      break;
    case WasmEdge_ValType_F32:
      Params[i] = WasmEdge_ValueGenF32(params_list[i].cast<float>());
      break;
    case WasmEdge_ValType_F64:
      Params[i] = WasmEdge_ValueGenF32(params_list[i].cast<double>());
      break;
    case WasmEdge_ValType_V128:
      Params[i] = WasmEdge_ValueGenV128(params_list[i].cast<int128_t>());
      break;
    case WasmEdge_ValType_FuncRef:
      Params[i] = WasmEdge_ValueGenFuncRef(params_list[i].cast<uint32_t>());
      break;
    // TODO: Handle Pointer
    // case WasmEdge_ValType_ExternRef:
    //   Params[i] = WasmEdge_ValueGenExternRef(
    //       params_list[i].cast<(void *)>());
    //   break;
    default:
      break;
    }
  }

  auto ret_len = WasmEdge_FunctionTypeGetReturnsLength(FuncTypeCxt);
  WasmEdge_ValType val_type_list_ret[ret_len];
  if (ret_len != WasmEdge_FunctionTypeGetReturns(FuncTypeCxt, val_type_list_ret,
                                                 ret_len)) {
    /* TODO: Handle errors gracefully */
    WasmEdge_FunctionTypeDelete(FuncTypeCxt);
    return pybind11::make_tuple(NULL, NULL);
  };

  WasmEdge_Value Returns[ret_len];
  WasmEdge_String funcName{(uint32_t)FuncName.length(), FuncName.c_str()};

  res =
      WasmEdge_VMExecute(VMCxt, funcName, Params, param_len, Returns, ret_len);

  for (int i = 0; i < ret_len; i++) {
    switch (val_type_list_ret[i]) {
    case WasmEdge_ValType_I32:
      returns.append(pybind11::cast(WasmEdge_ValueGetI32(Returns[i])));
      break;
    case WasmEdge_ValType_I64:
      returns.append(pybind11::cast(WasmEdge_ValueGetI64(Returns[i])));
      break;
    case WasmEdge_ValType_F32:
      returns.append(pybind11::cast(WasmEdge_ValueGetF32(Returns[i])));
      break;
    case WasmEdge_ValType_F64:
      returns.append(pybind11::cast(WasmEdge_ValueGetF64(Returns[i])));
      break;
    case WasmEdge_ValType_V128:
      returns.append(pybind11::cast(WasmEdge_ValueGetV128(Returns[i])));
      break;
    case WasmEdge_ValType_FuncRef:
      returns.append(pybind11::cast(WasmEdge_ValueGetFuncIdx(Returns[i])));
      break;
    // TODO: Handle Void Pointer
    // case WasmEdge_ValType_ExternRef:
    //   returns.append(pybind11::cast(WasmEdge_ValueGetExternRef(Returns[i])));
    //   break;
    default:
      break;
    }
  }

  // WasmEdge_FunctionTypeDelete(FuncTypeCxt);
  return pybind11::make_tuple(res, returns);
}

/* --------------- VM End -------------------------------- */

/* --------------- Store -------------------------------- */

pysdk::Store::Store() { StoreCxt = WasmEdge_StoreCreate(); }

pysdk::Store::~Store() { WasmEdge_StoreDelete(StoreCxt); }

WasmEdge_StoreContext *pysdk::Store::get() { return this->StoreCxt; }

pybind11::list pysdk::Store::listFunctions(int len) {
  pybind11::list ret;

  auto FuncNum = WasmEdge_StoreListFunctionLength(StoreCxt);
  WasmEdge_String FuncNames[len];

  auto GotFuncNum = WasmEdge_StoreListFunction(StoreCxt, FuncNames, FuncNum);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char *temp;
    WasmEdge_StringCopy(FuncNames[I], temp, FuncNames[I].Length);
    ret.append(temp);
    WasmEdge_StringDelete(FuncNames[I]);
  }

  return std::move(ret);
}

pybind11::list pysdk::Store::listModules(int len) {
  pybind11::list ret;

  auto ModNum = WasmEdge_StoreListModuleLength(StoreCxt);
  WasmEdge_String ModNames[len];

  auto GotFuncNum = WasmEdge_StoreListModule(StoreCxt, ModNames, ModNum);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char *temp;
    WasmEdge_StringCopy(ModNames[I], temp, ModNames[I].Length);
    ret.append(temp);
    WasmEdge_StringDelete(ModNames[I]);
  }

  return std::move(ret);
}

/* --------------- Store End -------------------------------- */

/* --------------- Configure -------------------------------- */

pysdk::Configure::Configure() { ConfCxt = WasmEdge_ConfigureCreate(); }

pysdk::Configure::~Configure() { WasmEdge_ConfigureDelete(ConfCxt); }

WasmEdge_ConfigureContext *pysdk::Configure::get() { return this->ConfCxt; }

void pysdk::Configure::add(WasmEdge_Proposal prop) {
  WasmEdge_ConfigureAddProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::remove(WasmEdge_Proposal prop) {
  WasmEdge_ConfigureRemoveProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::add(WasmEdge_HostRegistration hr) {
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::remove(WasmEdge_HostRegistration hr) {
  WasmEdge_ConfigureRemoveHostRegistration(ConfCxt,
                                           (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::set_max_paging(uint32_t max_memory) {
  WasmEdge_ConfigureSetMaxMemoryPage(ConfCxt, max_memory);
}

uint32_t pysdk::Configure::get_max_paging() {
  return WasmEdge_ConfigureGetMaxMemoryPage(ConfCxt);
}

void pysdk::Configure::set_opt_level(WasmEdge_CompilerOptimizationLevel level) {
  WasmEdge_ConfigureCompilerSetOptimizationLevel(ConfCxt, level);
}

WasmEdge_CompilerOptimizationLevel pysdk::Configure::get_opt_level() {
  return WasmEdge_ConfigureCompilerGetOptimizationLevel(ConfCxt);
}

/* --------------- Configure End -------------------------------- */

/* --------------- Result ----------------------------------------*/

pysdk::result::result() {}

pysdk::result::result(WasmEdge_Result res) { Res = res; }

pysdk::result::operator bool() { return WasmEdge_ResultOK(Res); }

const char *pysdk::result::message() { return WasmEdge_ResultGetMessage(Res); }

int pysdk::result::get_code() { return WasmEdge_ResultGetCode(Res); }

/* --------------- Result End ----------------------------------------*/

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
  pybind11::class_<WasmEdge_Value>(module, "Value")
      .def(pybind11::init())
      .def_readwrite("Value", &WasmEdge_Value::Value)
      .def_readwrite("Type", &WasmEdge_Value::Type);

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
      .def("listModules", &pysdk::Store::listModules);

  /*Overloading VM run functions*/

  pybind11::tuple (pysdk::VM::*run_step_by_step)(
      pybind11::object, pybind11::object, pybind11::object) = &pysdk::VM::run;
  pybind11::tuple (pysdk::VM::*run)(pybind11::object, pybind11::object,
                                    pybind11::object, pybind11::object,
                                    pybind11::object) = &pysdk::VM::run;

  pybind11::class_<pysdk::VM>(module, "VM")
      .def(pybind11::init())
      .def(pybind11::init<pysdk::Configure &>())
      .def(pybind11::init<pysdk::Store &>())
      .def(pybind11::init<pysdk::Configure &, pysdk::Store &>())
      .def("__doc__", &pysdk::VM::doc)
      .def("run", run)
      .def("run", run_step_by_step);
};
