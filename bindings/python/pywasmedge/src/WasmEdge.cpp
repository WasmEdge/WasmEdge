#include "WasmEdge.hpp"

pysdk::context_wrapper::context_wrapper() {
  ConfCxt = WasmEdge_ConfigureCreate();
}

pysdk::context_wrapper::~context_wrapper() {
  WasmEdge_ConfigureDelete(ConfCxt);
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

  class_<pysdk::context_wrapper>("Context", init<>())
      .def("__str__", &pysdk::context_wrapper::str);

  class_<pysdk::result>("Result", init<>())
      .def("__str__", &pysdk::result::str)
      .def("__bool__", &pysdk::result::operator bool)
      .def("message", &pysdk::result::message)
      .def("code", &pysdk::result::get_code);
};
