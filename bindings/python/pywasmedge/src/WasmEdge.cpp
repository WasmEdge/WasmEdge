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

boost::python::tuple pysdk::VM::run(boost::python::object _FileName,
                                    boost::python::object _FuncName,
                                    boost::python::object _params,
                                    boost::python::object _ret_len) {
  char const *FileName = boost::python::extract<char const *>(_FileName);
  char const *FuncName = boost::python::extract<char const *>(_FuncName);
  const int ret_len = boost::python::extract<int>(_ret_len);

  auto param_len = boost::python::len(_params);
  WasmEdge_Value Params[param_len];
  WasmEdge_Value Returns[ret_len];
  for (int i = 0; i < param_len; i++) {
    Params[i] =
        WasmEdge_ValueGenI32(boost::python::extract<int32_t>(_params[i]));
  }

  WasmEdge_String funcName{(uint32_t)strlen(FuncName), FuncName};
  pysdk::result res(WasmEdge_VMRunWasmFromFile(
      VMCxt, FileName, funcName, Params, param_len, Returns, ret_len));

  boost::python::list returns;
  for (int i = 0; i < ret_len; i++) {
    returns.append(std::to_string(WasmEdge_ValueGetI32(Returns[i])));
  }
  return boost::python::make_tuple(res, returns);
}

/* Step by Step execution */
boost::python::tuple pysdk::VM::run(boost::python::object _FileName,
                                    boost::python::object _FuncName,
                                    boost::python::object _params) {
  boost::python::list returns;

  char const *FileName = boost::python::extract<char const *>(_FileName);
  char const *FuncName = boost::python::extract<char const *>(_FuncName);

  pysdk::result res(WasmEdge_VMLoadWasmFromFile(VMCxt, FileName));
  if (!res) {
    /* TODO: Handle errors gracefully */
    return boost::python::make_tuple(res, NULL);
  }

  res = WasmEdge_VMValidate(VMCxt);
  if (!res) {
    /* TODO: Handle errors gracefully */
    return boost::python::make_tuple(res, NULL);
  }

  res = WasmEdge_VMInstantiate(VMCxt);
  if (!res) {
    /* TODO: Handle errors gracefully */
    return boost::python::make_tuple(res, NULL);
  }

  WasmEdge_FunctionTypeContext *FuncTypeCxt =
      (WasmEdge_FunctionTypeContext *)WasmEdge_VMGetFunctionType(
          VMCxt, WasmEdge_StringCreateByCString(FuncName));

  auto param_len = boost::python::len(_params);
  auto param_len_api = WasmEdge_FunctionTypeGetParametersLength(FuncTypeCxt);

  if (param_len != param_len_api) {
    /* TODO: Handle errors gracefully */
    WasmEdge_FunctionTypeDelete(FuncTypeCxt);
    return boost::python::make_tuple(NULL, NULL);
  }

  WasmEdge_ValType val_type_list_param[param_len];
  WasmEdge_FunctionTypeGetParameters(FuncTypeCxt, val_type_list_param,
                                     param_len);
  WasmEdge_Value Params[param_len];
  for (int i = 0; i < param_len; i++) {
    switch (val_type_list_param[i]) {
    case WasmEdge_ValType_I32:
      Params[i] =
          WasmEdge_ValueGenI32(boost::python::extract<int32_t>(_params[i]));
      break;
    case WasmEdge_ValType_I64:
      Params[i] =
          WasmEdge_ValueGenI64(boost::python::extract<int64_t>(_params[i]));
      break;
    case WasmEdge_ValType_F32:
      Params[i] =
          WasmEdge_ValueGenF32(boost::python::extract<_Float32>(_params[i]));
      break;
    case WasmEdge_ValType_F64:
      Params[i] =
          WasmEdge_ValueGenF32(boost::python::extract<_Float64>(_params[i]));
      break;
    case WasmEdge_ValType_V128:
      Params[i] =
          WasmEdge_ValueGenV128(boost::python::extract<int128_t>(_params[i]));
      break;
    case WasmEdge_ValType_FuncRef:
      Params[i] = WasmEdge_ValueGenFuncRef(
          boost::python::extract<uint32_t>(_params[i]));
      break;
    // TODO: Handle Pointer
    // case WasmEdge_ValType_ExternRef:
    //   Params[i] = WasmEdge_ValueGenExternRef(
    //       boost::python::extract<(void *)>(_params[i]));
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
    return boost::python::make_tuple(NULL, NULL);
  };

  WasmEdge_Value Returns[ret_len];
  WasmEdge_String funcName{(uint32_t)strlen(FuncName), FuncName};

  res =
      WasmEdge_VMExecute(VMCxt, funcName, Params, param_len, Returns, ret_len);

  for (int i = 0; i < ret_len; i++) {
    switch (val_type_list_ret[i]) {
    case WasmEdge_ValType_I32:
      returns.append(WasmEdge_ValueGetI32(Returns[i]));
      break;
    case WasmEdge_ValType_I64:
      returns.append(WasmEdge_ValueGetI64(Returns[i]));
      break;
    case WasmEdge_ValType_F32:
      returns.append(WasmEdge_ValueGetF32(Returns[i]));
      break;
    case WasmEdge_ValType_F64:
      returns.append(WasmEdge_ValueGetF64(Returns[i]));
      break;
    case WasmEdge_ValType_V128:
      returns.append(WasmEdge_ValueGetV128(Returns[i]));
      break;
    case WasmEdge_ValType_FuncRef:
      returns.append(WasmEdge_ValueGetFuncIdx(Returns[i]));
      break;
    case WasmEdge_ValType_ExternRef:
      returns.append(WasmEdge_ValueGetExternRef(Returns[i]));
      break;
    default:
      break;
    }
  }

  WasmEdge_FunctionTypeDelete(FuncTypeCxt);
  return boost::python::make_tuple(res, returns);
}

/* --------------- VM End -------------------------------- */

/* --------------- Store -------------------------------- */

pysdk::Store::Store() { StoreCxt = WasmEdge_StoreCreate(); }

pysdk::Store::~Store() { WasmEdge_StoreDelete(StoreCxt); }

WasmEdge_StoreContext *pysdk::Store::get() { return this->StoreCxt; }

boost::python::list pysdk::Store::listFunctions(int len) {
  boost::python::list ret;

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

boost::python::list pysdk::Store::listModules(int len) {
  boost::python::list ret;

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

/* --------------- Configure End -------------------------------- */

/* --------------- Result ----------------------------------------*/

pysdk::result::result() {}

pysdk::result::result(WasmEdge_Result res) { Res = res; }

pysdk::result::operator bool() { return WasmEdge_ResultOK(Res); }

const char *pysdk::result::message() { return WasmEdge_ResultGetMessage(Res); }

int pysdk::result::get_code() { return WasmEdge_ResultGetCode(Res); }

/* --------------- Result End ----------------------------------------*/

using namespace boost::python;

BOOST_PYTHON_MODULE(WasmEdge) {

  def("version", WasmEdge_VersionGet);

  class_<pysdk::logging>("Logging")
      .def("__doc__", &pysdk::logging::doc)
      .def("__str__", &pysdk::logging::str)
      .def("error", &pysdk::logging::error)
      .staticmethod("error")
      .def("debug", &pysdk::logging::debug)
      .staticmethod("debug");

  /*Overloading Python add and remove functions for Configure class*/

  void (pysdk::Configure::*add_prop)(WasmEdge_Proposal) =
      &pysdk::Configure::add;
  void (pysdk::Configure::*remove_prop)(WasmEdge_Proposal) =
      &pysdk::Configure::remove;
  void (pysdk::Configure::*add_host)(WasmEdge_HostRegistration) =
      &pysdk::Configure::add;
  void (pysdk::Configure::*remove_host)(WasmEdge_HostRegistration) =
      &pysdk::Configure::remove;

  class_<pysdk::Configure>("Configure", init<>())
      .def("__doc__", &pysdk::Configure::doc)
      .def("add", add_prop)
      .def("remove", remove_prop)
      .def("add", add_host)
      .def("remove", remove_host);

  /* WasmEdge WASM value struct. */
  enum_<WasmEdge_ValType>("Type")
      .value("I32", WasmEdge_ValType_I32)
      .value("I64", WasmEdge_ValType_I64)
      .value("F32", WasmEdge_ValType_F32)
      .value("F64", WasmEdge_ValType_F64)
      .value("V128", WasmEdge_ValType_V128)
      .value("FuncRef", WasmEdge_ValType_FuncRef)
      .value("ExternRef", WasmEdge_ValType_ExternRef)
      .export_values();

  class_<pysdk::result>("Result", init<>())
      .def("__doc__", &pysdk::result::doc)
      .def("__str__", &pysdk::result::message)
      .def("__bool__", &pysdk::result::operator bool)
      .def("message", &pysdk::result::message)
      .def("code", &pysdk::result::get_code);

  enum_<WasmEdge_Proposal>("Proposal")
      .value("BulkMemoryOperations", WasmEdge_Proposal_ImportExportMutGlobals)
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

  enum_<WasmEdge_HostRegistration>("Host")
      .value("Wasi", WasmEdge_HostRegistration_Wasi)
      .value("WasmEdge", WasmEdge_HostRegistration_WasmEdge_Process)
      .export_values();

  class_<pysdk::Store>("Store", init<>())
      .def("__doc__", &pysdk::Store::doc)
      .def("listFunctions", &pysdk::Store::listFunctions)
      .def("listModules", &pysdk::Store::listModules);

  /*Overloading VM run functions*/

  boost::python::tuple (pysdk::VM::*run_step_by_step)(object, object, object) =
      &pysdk::VM::run;
  boost::python::tuple (pysdk::VM::*run)(object, object, object, object) =
      &pysdk::VM::run;

  class_<pysdk::VM>("VM", init<>())
      .def(init<pysdk::Configure &>())
      .def(init<pysdk::Store &>())
      .def(init<pysdk::Configure &, pysdk::Store &>())
      .def("__doc__", &pysdk::VM::doc)
      .def("run", run)
      .def("run", run_step_by_step);
};
