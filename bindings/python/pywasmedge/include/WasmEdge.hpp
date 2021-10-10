#ifndef PY_WASMEDGE_H
#define PY_WASMEDGE_H

#include <boost/python.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <wasmedge.h>

namespace pysdk {

const char *logging_str =
    "The WasmEdge_LogSetErrorLevel() and "
    "WasmEdge_LogSetDebugLevel() APIs can set the logging "
    "system to debug level or error level. By default, the "
    "error level is set, and the debug info is hidden.";

struct logging {
  const char *str() { return pysdk::logging_str; };
  static void error() { WasmEdge_LogSetErrorLevel(); };
  static void debug() { WasmEdge_LogSetDebugLevel(); };
};

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

const char *Configure_str =
    "The objects, such as VM, Store, and HostFunction, are composed of "
    "Contexts. All of the contexts can be created by calling the corresponding "
    "creation APIs and should be destroyed by calling the corresponding "
    "deletion APIs. Developers have responsibilities to manage the contexts "
    "for memory management.";

class Configure {
private:
  WasmEdge_ConfigureContext *ConfCxt;

public:
  Configure();
  const char *str() { return pysdk::Configure_str; }
  void add(pysdk::WasmEdge_Proposal);
  void remove(pysdk::WasmEdge_Proposal);
  ~Configure();
};

const char *result_str =
    "The WasmEdge_Result object specifies the execution status. APIs about "
    "WASM execution will return the WasmEdge_Result to denote the status.";

class result {
private:
  WasmEdge_Result Res;

public:
  result();
  const char *str() { return pysdk::result_str; }
  explicit operator bool();
  const char *message();

  int get_code();
};

} // namespace pysdk

#endif // PY_WASMEDGE_H