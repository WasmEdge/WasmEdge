#ifndef PY_WASMEDGE_H
#define PY_WASMEDGE_H

#include <boost/python.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <doc_strings.hpp>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <wasmedge.h>

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

enum WasmEdge_Proposal {
  WasmEdge_Proposal_BulkMemoryOperations = 0,
  WasmEdge_Proposal_ReferenceTypes,
  WasmEdge_Proposal_SIMD,
  WasmEdge_Proposal_TailCall,
  WasmEdge_Proposal_Annotations,
  WasmEdge_Proposal_Memory64,
  WasmEdge_Proposal_Threads,
  WasmEdge_Proposal_ExceptionHandling,
  WasmEdge_Proposal_FunctionReferences
};

/*This configuration is used for the VM context to turn on the WASI or
 * wasmedge_process supports and only effective in VM contexts.*/
enum WasmEdge_HostRegistration {
  WasmEdge_HostRegistration_Wasi = 0,
  WasmEdge_HostRegistration_WasmEdge_Process
};

class Configure {
private:
  WasmEdge_ConfigureContext *ConfCxt;

public:
  Configure();
  ~Configure();
  const char *doc() { return pysdk::Configure_doc; }
  WasmEdge_ConfigureContext *get();
  void add(pysdk::WasmEdge_Proposal);
  void add(pysdk::WasmEdge_HostRegistration);
  void remove(pysdk::WasmEdge_Proposal);
  void remove(pysdk::WasmEdge_HostRegistration);
};

class Store {
private:
  WasmEdge_StoreContext *StoreCxt;

public:
  Store();
  ~Store();
  const char *doc() { return pysdk::Store_doc; }
  WasmEdge_StoreContext *get();
  boost::python::list listFunctions(int);
  boost::python::list listModules(int);
};

class result {
private:
  WasmEdge_Result Res;

public:
  result();
  result(WasmEdge_Result);
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

  boost::python::tuple run(boost::python::object, boost::python::object,
                           boost::python::object, boost::python::object);
};

} // namespace pysdk

#endif // PY_WASMEDGE_H