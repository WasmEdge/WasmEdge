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

void pysdk::Configure::add(pysdk::WasmEdge_Proposal prop) {
  WasmEdge_ConfigureAddProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::remove(pysdk::WasmEdge_Proposal prop) {
  WasmEdge_ConfigureRemoveProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::add(pysdk::WasmEdge_HostRegistration hr) {
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        (::WasmEdge_HostRegistration)hr);
}

void pysdk::Configure::remove(pysdk::WasmEdge_HostRegistration hr) {
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

  void (pysdk::Configure::*add_prop)(pysdk::WasmEdge_Proposal) =
      &pysdk::Configure::add;
  void (pysdk::Configure::*remove_prop)(pysdk::WasmEdge_Proposal) =
      &pysdk::Configure::remove;
  void (pysdk::Configure::*add_host)(pysdk::WasmEdge_HostRegistration) =
      &pysdk::Configure::add;
  void (pysdk::Configure::*remove_host)(pysdk::WasmEdge_HostRegistration) =
      &pysdk::Configure::remove;

  class_<pysdk::Configure>("Configure", init<>())
      .def("__doc__", &pysdk::Configure::doc)
      .def("add", add_prop)
      .def("remove", remove_prop)
      .def("add", add_host)
      .def("remove", remove_host);

  class_<pysdk::result>("Result", init<>())
      .def("__doc__", &pysdk::result::doc)
      .def("__str__", &pysdk::result::message)
      .def("__bool__", &pysdk::result::operator bool)
      .def("message", &pysdk::result::message)
      .def("code", &pysdk::result::get_code);

  enum_<pysdk::WasmEdge_Proposal>("Proposal")
      .value("BulkMemoryOperations",
             pysdk::WasmEdge_Proposal_BulkMemoryOperations)
      .value("ReferenceTypes", pysdk::WasmEdge_Proposal_ReferenceTypes)
      .value("SIMD", pysdk::WasmEdge_Proposal_SIMD)
      .value("TailCall", pysdk::WasmEdge_Proposal_TailCall)
      .value("Annotations", pysdk::WasmEdge_Proposal_Annotations)
      .value("Memory64", pysdk::WasmEdge_Proposal_Memory64)
      .value("Threads", pysdk::WasmEdge_Proposal_Threads)
      .value("ExceptionHandling", pysdk::WasmEdge_Proposal_ExceptionHandling)
      .value("FunctionReferences", pysdk::WasmEdge_Proposal_FunctionReferences)
      .export_values();

  enum_<pysdk::WasmEdge_HostRegistration>("Host")
      .value("Wasi", pysdk::WasmEdge_HostRegistration_Wasi)
      .value("WasmEdge", pysdk::WasmEdge_HostRegistration_WasmEdge_Process)
      .export_values();

  class_<pysdk::Store>("Store", init<>())
      .def("__doc__", &pysdk::Store::doc)
      .def("listFunctions", &pysdk::Store::listFunctions)
      .def("listModules", &pysdk::Store::listModules);

  class_<pysdk::VM>("VM", init<>())
      .def(init<pysdk::Configure &>())
      .def(init<pysdk::Store &>())
      .def(init<pysdk::Configure &, pysdk::Store &>())
      .def("__doc__", &pysdk::VM::doc)
      .def("run", &pysdk::VM::run);
};
