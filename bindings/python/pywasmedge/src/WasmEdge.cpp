#include "WasmEdge.hpp"

pysdk::Configure::Configure() { ConfCxt = WasmEdge_ConfigureCreate(); }

pysdk::Configure::~Configure() { WasmEdge_ConfigureDelete(ConfCxt); }

void pysdk::Configure::add(pysdk::WasmEdge_Proposal prop) {
  WasmEdge_ConfigureAddProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

void pysdk::Configure::remove(pysdk::WasmEdge_Proposal prop) {
  WasmEdge_ConfigureRemoveProposal(ConfCxt, (::WasmEdge_Proposal)prop);
}

pysdk::result::result() {}

pysdk::result::operator bool() { return WasmEdge_ResultOK(Res); }

const char *pysdk::result::message() { return WasmEdge_ResultGetMessage(Res); }

int pysdk::result::get_code() { return WasmEdge_ResultGetCode(Res); }

using namespace boost::python;

BOOST_PYTHON_MODULE(WasmEdge) {

  def("version", WasmEdge_VersionGet);

  class_<pysdk::logging>("Logging")
      .def("__str__", &pysdk::logging::str)
      .def("error", &pysdk::logging::error)
      .staticmethod("error")
      .def("debug", &pysdk::logging::debug)
      .staticmethod("debug");

  class_<pysdk::Configure>("Configure", init<>())
      .def("__str__", &pysdk::Configure::str)
      .def("add", &pysdk::Configure::add)
      .def("remove", &pysdk::Configure::remove);

  class_<pysdk::result>("Result", init<>())
      .def("__str__", &pysdk::result::str)
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
};
