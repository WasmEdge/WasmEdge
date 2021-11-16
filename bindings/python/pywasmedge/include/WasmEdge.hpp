#ifndef PY_WASMEDGE_H
#define PY_WASMEDGE_H

#include "pybind11/pybind11.h"
#include <doc_strings.hpp>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <wasmedge/enum_configure.h>
#include <wasmedge/wasmedge.h>

namespace pysdk {

struct logging {
  const char *str() { return _str.c_str(); };
  static void error() {
    _str = "logging: Level=error";
    WasmEdge_LogSetErrorLevel();
  };
  static void debug() {
    _str = "logging: Level=debug";
    WasmEdge_LogSetDebugLevel();
  };
  static std::string _str;
  const char *doc() { return pysdk::logging_doc; }
};

std::string logging::_str = "logging: Level not set";

class Configure {
private:
  WasmEdge_ConfigureContext *ConfCxt;

public:
  Configure();
  ~Configure();
  const char *doc() { return pysdk::Configure_doc; }
  WasmEdge_ConfigureContext *get();
  void add(WasmEdge_Proposal);
  void add(WasmEdge_HostRegistration);
  void remove(WasmEdge_Proposal);
  void remove(WasmEdge_HostRegistration);
  void set_max_paging(uint32_t);
  uint32_t get_max_paging();
  void set_opt_level(WasmEdge_CompilerOptimizationLevel);
  WasmEdge_CompilerOptimizationLevel get_opt_level();
};

class Store {
private:
  WasmEdge_StoreContext *StoreCxt;

public:
  Store();
  ~Store();
  const char *doc() { return pysdk::Store_doc; }
  WasmEdge_StoreContext *get();
  pybind11::list listFunctions(int);
  pybind11::list listModules(int);
};

class result {
private:
  WasmEdge_Result Res;

public:
  result();
  result(WasmEdge_Result);
  void operator=(const WasmEdge_Result &res) { Res = res; }
  const char *doc() { return pysdk::result_doc; }
  explicit operator bool();
  const char *message();

  int get_code();
};

class VM {
private:
  WasmEdge_VMContext *VMCxt;

public:
  VM();
  VM(Store &);
  VM(Configure &);
  VM(Configure &, Store &);
  ~VM();
  const char *doc() { return pysdk::vm_doc; };

  pybind11::tuple run(pybind11::object, pybind11::object, pybind11::object,
                      pybind11::object, pybind11::object);
  pybind11::tuple run(pybind11::object, pybind11::object, pybind11::object);
};

} // namespace pysdk

#endif // PY_WASMEDGE_H