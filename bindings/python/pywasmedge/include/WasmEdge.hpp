#ifndef PY_WASMEDGE_H
#define PY_WASMEDGE_H

#include "pybind11/pybind11.h"
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <wasmedge/enum_configure.h>
#include <wasmedge/wasmedge.h>

namespace pysdk {

template <typename T> class base {
protected:
  T *context;
  bool _del = true;

public:
  base() = default;
  base(T *cxt, bool del = false) : context(cxt), _del(del) {}
  base(const T *cxt) : context(const_cast<T *>(cxt)) { _del = false; }
  virtual ~base() = default;

  const T &get() const { return const_cast<const T *>(context); }
  T *get() { return context; }
};

class Memory;

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
};

class Value {
private:
  WasmEdge_Value Val;
  pybind11::object obj;

public:
  Value(pybind11::object, WasmEdge_ValType &);
  Value(WasmEdge_Value *);
  Value(const WasmEdge_Value &);
  ~Value();
  void set_value(pybind11::object, WasmEdge_ValType &);
  pybind11::object get_value();
  WasmEdge_ValType get_type();
  WasmEdge_Value get();
};

class result {
private:
  WasmEdge_Result Res;

public:
  result();
  result(WasmEdge_Result);
  result(int &);
  void operator=(const WasmEdge_Result &res) { Res = res; }
  explicit operator bool();
  const char *message();
  WasmEdge_Result get();
  int get_code();
};

WasmEdge_Result host_function(void *, WasmEdge_MemoryInstanceContext *,
                              const WasmEdge_Value *, WasmEdge_Value *);

struct function_utility {
  size_t param_len;
  pybind11::function func;
};

class FunctionTypeContext : public base<WasmEdge_FunctionTypeContext> {
public:
  FunctionTypeContext(pybind11::list, pybind11::list);
  FunctionTypeContext(const WasmEdge_FunctionTypeContext *);
  ~FunctionTypeContext() override;
  uint32_t get_param_len();
  pybind11::list get_param_types(const uint32_t &);
  uint32_t get_ret_len();
  pybind11::list get_ret_types(const uint32_t &);
};

class Function : public base<WasmEdge_FunctionInstanceContext> {
private:
  function_utility *func_util;

public:
  Function(FunctionTypeContext &, pybind11::function, uint64_t &);
  Function(const WasmEdge_FunctionInstanceContext *);
  Function(WasmEdge_FunctionInstanceContext *);
  ~Function() override;
  FunctionTypeContext get_func_type();
};

class GlobalTypeCxt : public base<WasmEdge_GlobalTypeContext> {
public:
  GlobalTypeCxt(const WasmEdge_ValType &, const WasmEdge_Mutability &);
  GlobalTypeCxt(const WasmEdge_GlobalTypeContext *);
  ~GlobalTypeCxt() override;
  WasmEdge_Mutability GetMutability();
  WasmEdge_ValType GetValType();
};

class Global : public base<WasmEdge_GlobalInstanceContext> {

public:
  Global(GlobalTypeCxt &, Value &);
  Global(const WasmEdge_GlobalInstanceContext *);
  ~Global() override;
  GlobalTypeCxt GetGlobalType();
  Value GetValue();
  void SetValue(Value &);
};

class MemoryTypeCxt : public base<WasmEdge_MemoryTypeContext> {
public:
  MemoryTypeCxt(WasmEdge_Limit &);
  MemoryTypeCxt(const WasmEdge_MemoryTypeContext *);
  ~MemoryTypeCxt() override;
};

class Memory : public base<WasmEdge_MemoryInstanceContext> {
public:
  Memory(MemoryTypeCxt &);
  Memory(const WasmEdge_MemoryInstanceContext *);
  Memory(WasmEdge_MemoryInstanceContext *);
  ~Memory() override;
  result set_data(pybind11::tuple, const uint32_t &);
  uint32_t get_page_size();
  result grow_page(const uint32_t &);
  pybind11::tuple get_data(const uint32_t &, const uint32_t &);
  MemoryTypeCxt get_type();
};

class TableTypeCxt : public base<WasmEdge_TableTypeContext> {
public:
  TableTypeCxt(WasmEdge_RefType &, WasmEdge_Limit &);
  TableTypeCxt(const WasmEdge_TableTypeContext *);
  ~TableTypeCxt() override;
  WasmEdge_Limit GetLimit();
  WasmEdge_RefType GetRefType();
};

class Table {
private:
  WasmEdge_TableInstanceContext *HostTable;
  bool delete_cxt = true;

public:
  Table(TableTypeCxt &);
  Table(WasmEdge_TableInstanceContext *, bool);
  ~Table();
  WasmEdge_TableInstanceContext *get();
  TableTypeCxt get_type();
  uint32_t get_size();
  result grow_size(const uint32_t &);
  result set_data(Value &, const uint32_t &);
  pybind11::tuple get_data(const uint32_t &);
};

class Configure {
private:
  WasmEdge_ConfigureContext *ConfCxt;

public:
  Configure();
  ~Configure();
  WasmEdge_ConfigureContext *get();
  void AddProposal(WasmEdge_Proposal &);
  void AddHostRegistration(WasmEdge_HostRegistration &);
  void RemoveProposal(WasmEdge_Proposal &);
  void RemoveHostRegistration(WasmEdge_HostRegistration &);
  WasmEdge_CompilerOptimizationLevel CompilerGetOptimizationLevel();
  WasmEdge_CompilerOutputFormat CompilerGetOutputFormat();
  bool CompilerIsDumpIR();
  bool CompilerIsGenericBinary();
  bool CompilerIsInterruptible();
  void CompilerSetDumpIR(bool &);
  void CompilerSetGenericBinary(bool &);
  void CompilerSetInterruptible(bool &);
  void CompilerSetOptimizationLevel(WasmEdge_CompilerOptimizationLevel &);
  void CompilerSetOutputFormat(WasmEdge_CompilerOutputFormat &);
  uint32_t GetMaxMemoryPage();
  void SetMaxMemoryPage(uint32_t &);
  bool HasHostRegistration(WasmEdge_HostRegistration &);
  bool HasProposal(WasmEdge_Proposal &);
  bool StatisticsIsCostMeasuring();
  bool StatisticsIsInstructionCounting();
  bool StatisticsIsTimeMeasuring();
  void StatisticsSetCostMeasuring(bool &);
  void StatisticsSetInstructionCounting(bool &);
  void StatisticsSetTimeMeasuring(bool &);
};

class Store {
private:
  WasmEdge_StoreContext *StoreCxt;
  bool external = false;

public:
  Store();
  Store(WasmEdge_StoreContext *);
  ~Store();
  WasmEdge_StoreContext *get();
  pybind11::list ListFunction(uint32_t &);
  pybind11::list ListModule(uint32_t &);
  pybind11::list ListFunctionRegistered(const std::string &, uint32_t &);
  Memory FindMemory(std::string &);
  Function FindFunction(std::string &);
  Function FindFunctionRegistered(std::string &, std::string &);
  Global FindGlobal(std::string &);
  Global FindGlobalRegistered(std::string &, std::string &);
  Memory FindMemoryRegistered(std::string &, std::string &);
  Table FindTable(std::string &);
  Table FindTableRegistered(std::string &, std::string &);
  uint32_t ListFunctionLength();
  uint32_t ListFunctionRegisteredLength(std::string &);
  pybind11::list ListGlobal(uint32_t &);
  uint32_t ListGlobalLength();
  uint32_t ListGlobalRegisteredLength(std::string &);
  pybind11::list ListGlobalRegistered(std::string &, uint32_t &);
  pybind11::list ListMemory(uint32_t &);
  pybind11::list ListMemoryRegistered(std::string &, uint32_t &);
  uint32_t ListMemoryLength();
  uint32_t ListMemoryRegisteredLength(std::string &);
  uint32_t ListModuleLength();
  pybind11::list ListTable(uint32_t &);
  pybind11::list ListTableRegistered(std::string &, uint32_t &);
  uint32_t ListTableLength();
  uint32_t ListTableRegisteredLength(std::string &);
};

class ASTModuleCxt {
private:
  WasmEdge_ASTModuleContext *ASTCxt;
  bool external = false;

public:
  ASTModuleCxt();
  ASTModuleCxt(WasmEdge_ASTModuleContext *);
  ~ASTModuleCxt();
  WasmEdge_ASTModuleContext *get();
  WasmEdge_ASTModuleContext **get_addr();
  pybind11::list ListExports(uint32_t &);
  pybind11::list ListImports(uint32_t &);
  uint32_t ListImportsLength();
  uint32_t ListExportsLength();
};

class ExportType {
private:
  WasmEdge_ExportTypeContext *ExpoCxt;

public:
  ExportType();
  ExportType(WasmEdge_ExportTypeContext *);
  ~ExportType();
  WasmEdge_ExportTypeContext *get();
  std::string GetExternalName();
  WasmEdge_ExternalType GetExternalType();
  FunctionTypeContext GetFunctionType(ASTModuleCxt &);
  GlobalTypeCxt GetGlobalType(ASTModuleCxt &);
  MemoryTypeCxt GetMemoryType(ASTModuleCxt &);
};

class Loader {
private:
  WasmEdge_LoaderContext *LoadCxt;

public:
  Loader(Configure &);
  ~Loader();
  WasmEdge_LoaderContext *get();
  result parse(ASTModuleCxt &, std::string &);
  result parse(ASTModuleCxt &, pybind11::tuple);
};

class Validator {
private:
  WasmEdge_ValidatorContext *ValidCxt;

public:
  Validator(Configure &);
  ~Validator();
  WasmEdge_ValidatorContext *get();
  result validate(ASTModuleCxt &);
};

class Statistics {
private:
  WasmEdge_StatisticsContext *StatCxt;
  bool delete_stat = false;

public:
  Statistics();
  Statistics(WasmEdge_StatisticsContext *, bool);
  ~Statistics();
  uint64_t GetInstrCount();
  double GetInstrPerSecond();
  uint64_t GetTotalCost();
  void SetCostLimit(uint64_t &);
  void SetCostTable(pybind11::tuple);
};

class ImportTypeContext {
private:
  WasmEdge_ImportTypeContext *Cxt;

public:
  ImportTypeContext();
  ImportTypeContext(WasmEdge_ImportTypeContext *);
  ~ImportTypeContext();
  WasmEdge_ImportTypeContext *get();
  std::string get_external_name();
  WasmEdge_ExternalType get_external_type();
  FunctionTypeContext get_function_type_cxt(ASTModuleCxt &);
  GlobalTypeCxt get_global_type_cxt(ASTModuleCxt &);
  MemoryTypeCxt GetMemoryType(ASTModuleCxt &);
  std::string GetModuleName();
  TableTypeCxt GetTableType(ASTModuleCxt &);
};

class import_object {
private:
  WasmEdge_ImportObjectContext *ModCxt;

public:
  import_object(std::string &);
  import_object(pybind11::tuple, pybind11::tuple, pybind11::tuple);
  import_object(pybind11::tuple, bool &);
  import_object(WasmEdge_ImportObjectContext *);
  ~import_object();
  WasmEdge_ImportObjectContext *get();
  void AddFunction(std::string &, Function &);
  void AddGlobal(std::string &, Global &);
  void AddMemory(std::string &, Memory &);
  void AddTable(std::string &, Table &);
  void InitWASI(pybind11::tuple, pybind11::tuple, pybind11::tuple);
  void InitWasmEdgeProcess(pybind11::tuple, bool &);
  uint32_t WASIGetExitCode();
};

class Executor {
private:
  WasmEdge_ExecutorContext *ExecCxt;

public:
  Executor(Configure &);
  ~Executor();
  WasmEdge_ExecutorContext *get();
  result instantiate(Store &, ASTModuleCxt &);
  pybind11::tuple invoke(Store &, std::string &, pybind11::list);
  result RegisterImport(Store &, import_object &);
  result RegisterModule(Store &, ASTModuleCxt &, std::string &);
};

class Compiler {
private:
  WasmEdge_CompilerContext *cxt;

public:
  Compiler(Configure &);
  ~Compiler();
  result Compile(std::string &, std::string &);
};

class Async {
private:
  WasmEdge_Async *async;

public:
  Async();
  Async(WasmEdge_Async *);
  ~Async();
  WasmEdge_Async *get();
  pybind11::tuple Get(uint32_t &);
  uint32_t GetReturnsLength();
  void Wait();
  bool WaitFor(uint64_t &);
  void Cancel();
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

  result register_module_from_file(std::string &, std::string &);
  result register_module_from_ast(std::string &, ASTModuleCxt &);
  result register_module_from_buffer(std::string &, pybind11::tuple);
  result register_module_from_import_object(import_object &);

  pybind11::tuple execute_registered(std::string &, std::string &,
                                     pybind11::list, const uint32_t &);
  pybind11::tuple execute(std::string &, pybind11::tuple, uint32_t &);
  Async executeAsync(std::string &, pybind11::tuple);
  Async executeAsyncRegistered(std::string &, std::string &, pybind11::tuple);
  Async run_from_ast_async(pysdk::ASTModuleCxt &, std::string &,
                           pybind11::tuple);
  Async run_from_buffer_async(pybind11::tuple, pybind11::tuple, std::string &);
  Async run_from_wasm_file_async(std::string &, std::string &, pybind11::tuple);

  result validate();

  pybind11::dict get_functions(uint32_t &);
  uint32_t get_functions_len();
  FunctionTypeContext get_function_type(std::string &);
  FunctionTypeContext get_function_type_registered(std::string &,
                                                   std::string &);
  import_object get_import_module_context(WasmEdge_HostRegistration &);
  Statistics get_statistics_context();
  Store get_store_cxt();
  result instantiate();
  result load_from_ast(ASTModuleCxt &);
  result load_from_buffer(pybind11::tuple);
  result load_from_file(std::string &);

  pybind11::tuple run_from_ast(ASTModuleCxt &, std::string &, pybind11::tuple,
                               uint32_t &);
  pybind11::tuple run_from_buffer(pybind11::tuple, pybind11::tuple,
                                  std::string &, uint32_t &);
  pybind11::tuple run_from_wasm_file(std::string &, std::string &,
                                     pybind11::tuple, uint32_t &);
};

} // namespace pysdk

#endif // PY_WASMEDGE_H