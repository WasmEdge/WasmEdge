#ifndef PY_WASMEDGE_H
#define PY_WASMEDGE_H

#include "doc_strings.hpp"
#include "pybind11/pybind11.h"
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

class Value {
private:
  WasmEdge_Value Val;
  pybind11::object obj;

public:
  Value(pybind11::object, WasmEdge_ValType &);
  ~Value();
  void set_value(pybind11::object, WasmEdge_ValType &);
  pybind11::object get_value();
  WasmEdge_ValType get_type();
  WasmEdge_Value get();
};

class Ref {
private:
  WasmEdge_Value Val;
  void *Ptr;

public:
  Ref(pybind11::object type, pybind11::object obj = pybind11::none());
  ~Ref();
  pybind11::object get_py_obj();
  WasmEdge_ValType get_type();
  void *get();
  WasmEdge_Value get_val();
  uint32_t get_function_index();
  bool is_null();
};

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
  pybind11::list listFunctions();
  pybind11::list listModules();
  pybind11::list listRegisteredFunctions(const std::string &);
};

class ASTModuleCxt {
private:
  WasmEdge_ASTModuleContext *ASTCxt;

public:
  ASTModuleCxt();
  ~ASTModuleCxt();
  WasmEdge_ASTModuleContext *get();
  WasmEdge_ASTModuleContext **get_addr();
  pybind11::list listExports();
  pybind11::list listImports();
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

class Loader {
private:
  WasmEdge_LoaderContext *LoadCxt;

public:
  Loader(pysdk::Configure &);
  ~Loader();
  WasmEdge_LoaderContext *get();
  pysdk::result parse(pysdk::ASTModuleCxt &, std::string &);
  pysdk::result parse(pysdk::ASTModuleCxt &, pybind11::tuple);
};

class Validator {
private:
  WasmEdge_ValidatorContext *ValidCxt;

public:
  Validator(pysdk::Configure &);
  ~Validator();
  WasmEdge_ValidatorContext *get();
  pysdk::result validate(pysdk::ASTModuleCxt &);
};

class Executor {
private:
  WasmEdge_ExecutorContext *ExecCxt;

public:
  Executor(pysdk::Configure &);
  ~Executor();
  WasmEdge_ExecutorContext *get();
  pysdk::result instantiate(pysdk::Store &, pysdk::ASTModuleCxt &);
  pybind11::tuple invoke(pysdk::Store &, std::string &, pybind11::list);
};

WasmEdge_Result host_function(void *, WasmEdge_MemoryInstanceContext *,
                              const WasmEdge_Value *, WasmEdge_Value *);

struct function_utility {
  size_t param_len;
  pybind11::function func;
};

class function {
private:
  WasmEdge_FunctionTypeContext *HostFType;
  WasmEdge_FunctionInstanceContext *HostFuncCxt;
  size_t param_len;
  size_t ret_len;
  pybind11::function func;
  enum WasmEdge_ValType *param_types, *return_types;
  pysdk::function_utility *hfunc_util;

public:
  function(pybind11::function);
  ~function();
  WasmEdge_FunctionInstanceContext *get();
};

class MemoryTypeCxt {
private:
  WasmEdge_MemoryTypeContext *MemTypeCxt;

public:
  MemoryTypeCxt(WasmEdge_Limit &);
  ~MemoryTypeCxt();
  WasmEdge_MemoryTypeContext *get();
};

class GlobalTypeCxt {
private:
  WasmEdge_GlobalTypeContext *GlobTypeCxt;

public:
  GlobalTypeCxt(const WasmEdge_ValType &, const WasmEdge_Mutability &);
  ~GlobalTypeCxt();
  WasmEdge_GlobalTypeContext *get();
};

class import_object {
private:
  WasmEdge_ImportObjectContext *ModCxt;

public:
  import_object(std::string name = "Unnamed");
  ~import_object();
  WasmEdge_ImportObjectContext *get();
  void add(pysdk::function &, std::string name = "Function_Name");
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

  pybind11::tuple run(pybind11::object, pybind11::object, pybind11::object,
                      std::string &);

  pybind11::list list_exported_functions();

  pysdk::result register_module_from_file(std::string &, std::string &);
  pysdk::result register_module_from_ast(std::string &, pysdk::ASTModuleCxt &);
  pysdk::result register_module_from_buffer(std::string &, pybind11::tuple);
  pysdk::result register_module_from_import_object(pysdk::import_object &);
};

} // namespace pysdk

#endif // PY_WASMEDGE_H