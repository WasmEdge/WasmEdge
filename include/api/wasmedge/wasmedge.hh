// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/wasmedge.hh - WasmEdge C++ API ---------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the class declarations of WasmEdge C++ API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_CPP_API_HH
#define WASMEDGE_CPP_API_HH

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#ifdef WASMEDGE_COMPILE_LIBRARY
#define WASMEDGE_CPP_API_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_CPP_API_EXPORT __declspec(dllimport)
#endif // WASMEDGE_COMPILE_LIBRARY
#ifdef WASMEDGE_PLUGIN
#define WASMEDGE_CPP_API_PLUGIN_EXPORT __declspec(dllexport)
#else
#define WASMEDGE_CPP_API_PLUGIN_EXPORT __declspec(dllimport)
#endif // WASMEDGE_PLUGIN
#else
#define WASMEDGE_CPP_API_EXPORT __attribute__((visibility("default")))
#define WASMEDGE_CPP_API_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif // _WIN32

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "wasmedge/int128.h"
#include "wasmedge/version.h"

namespace WasmEdge {
namespace SDK {

enum WASMEDGE_CPP_API_EXPORT ErrCategory { WASM = 0x00, UserLevelError = 0x01 };

enum WASMEDGE_CPP_API_EXPORT HostRegistration {
  Wasi,
  WasmEdge_Process,
};

enum WASMEDGE_CPP_API_EXPORT Proposal {
  ImportExportMutGlobals = 0,
  NonTrapFloatToIntConversions,
  SignExtensionOperators,
  MultiValue,
  BulkMemoryOperations,
  ReferenceTypes,
  SIMD,
  TailCall,
  MultiMemories,
  Annotations,
  Memory64,
  ExceptionHandling,
  ExtendedConst,
  Threads,
  FunctionReferences
};

enum WASMEDGE_CPP_API_EXPORT CompilerOutputFormat {
  // Native dynamic library format.
  Native = 0,
  // WebAssembly with AOT compiled codes in custom sections.
  Wasm
};

enum WASMEDGE_CPP_API_EXPORT CompilerOptimizationLevel {
  // Disable as many optimizations as possible.
  Level_O0 = 0,
  // Optimize quickly without destroying debuggability.
  Level_O1,
  // Optimize for fast execution as much as possible without triggering
  // significant incremental compile time or code size growth.
  Level_O2,
  // Optimize for fast execution as much as possible.
  Level_O3,
  // Optimize for small code size as much as possible without triggering
  // significant incremental compile time or execution time slowdowns.
  Level_Os,
  // Optimize for small code size as much as possible.
  Level_Oz
};

enum WASMEDGE_CPP_API_EXPORT ValType {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  V128 = 0x7B,
  FuncRef = 0x70,
  ExternRef = 0x6F
};

enum WASMEDGE_CPP_API_EXPORT ExternalType {
  Function = 0x00U,
  Table = 0x01U,
  Memory = 0x02U,
  Global = 0x03U
};

enum WASMEDGE_CPP_API_EXPORT Mutability { Const = 0x00, Var = 0x01 };

// TODO: ValType is an extension of RefType
enum WASMEDGE_CPP_API_EXPORT RefType { FuncRef = 0x70, ExternRef = 0x6F };

class WASMEDGE_CPP_API_EXPORT Version {
public:
  static std::string Get();
  static uint32_t GetMajor();
  static uint32_t GetMinor();
  static uint32_t GetPatch();

private:
  Version() = default;
  ~Version() = default;
};

class WASMEDGE_CPP_API_EXPORT Log {
public:
  static void SetErrorLevel();
  static void SetDebugLevel();
  static void Off();

private:
  Log() = default;
  ~Log() = default;
};

class WASMEDGE_CPP_API_EXPORT Value {
public:
  Value(const int32_t Val);
  Value(const int64_t Val);
  Value(const float Val);
  Value(const double Val);
  Value(const int128_t Val);
  Value(const RefType T);
  Value(const FunctionInstance &Cxt);
  Value(std::shared_ptr<void> ExternRef);
  ~Value() = default;

  int32_t GetI32();
  int64_t GetI64();
  float GetF32();
  double GetF64();
  int128_t GetV128();
  const FunctionInstance GetFuncRef();
  std::shared_ptr<void> GetExternRef();

  bool IsNullRef();

  class ValueUtils;

private:
  uint128_t Val;
  ValType Type;
  Value(uint128_t Val, ValType Type) : Val(Val), Type(Type) {}

  friend class Async;
};

class WASMEDGE_CPP_API_EXPORT Result {
public:
  Result(const ErrCategory Category, const uint32_t Code);
  ~Result() = default;

  // static methods
  static Result Success() { return Result{0x00}; }
  static Result Terminate() { return Result{0x01}; }
  static Result Fail() { return Result{0x02}; }

  bool IsOk();
  uint32_t GetCode();
  ErrCategory GetCategory();
  const std::string GetMessage();

  class ResultFactory;

private:
  uint32_t Code;
  Result(const uint32_t Code) : Code(Code) {}

  friend class Async;
};

// >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT Limit {
public:
  Limit(bool HasMax = false, bool Shared = false, uint32_t Min = 0,
        uint32_t Max = 0)
      : HasMax(HasMax), Shared(Shared), Min(Min), Max(Max){};
  ~Limit() = default;

  bool HasMax;
  bool Shared;
  uint32_t Min;
  uint32_t Max;

  bool operator==(const Limit &Lim);
};

class WASMEDGE_CPP_API_EXPORT FunctionType {
protected:
  FunctionType() = default;
  ~FunctionType() = default;

public:
  static FunctionType New(const std::vector<ValType> &ParamList,
                          const std::vector<ValType> &ReturnList);

  const std::vector<ValType> GetParameters();
  const std::vector<ValType> GetReturns();

  friend class ImportType;
  friend class ExportType;
  friend class VM;
};

class WASMEDGE_CPP_API_EXPORT TableType {
protected:
  TableType() = default;
  ~TableType() = default;

public:
  static TableType New(const RefType RefType, const Limit &Limit);

  RefType GetRefType();
  const Limit GetLimit();

  friend class ImportType;
  friend class ExportType;
};

class WASMEDGE_CPP_API_EXPORT MemoryType {
protected:
  MemoryType() = default;
  ~MemoryType() = default;

public:
  static MemoryType New(const Limit &Limit);
  const Limit GetLimit();

  friend class ImportType;
  friend class ExportType;
};

class WASMEDGE_CPP_API_EXPORT GlobalType {
protected:
  GlobalType() = default;
  ~GlobalType() = default;

public:
  static GlobalType New(const ValType, const Mutability Mut);

  ValType GetValType();
  Mutability GetMutability();

  friend class ImportType;
  friend class ExportType;
};

class WASMEDGE_CPP_API_EXPORT ImportType {
public:
  ExternalType GetExternalType();
  std::string GetModuleName();
  std::string GetExternalName();
  const FunctionType GetFunctionType(const ASTModule &ASTCxt);
  const TableType GetTableType(const ASTModule &ASTCxt);
  const MemoryType GetMemoryType(const ASTModule &ASTCxt);
  const GlobalType GetGlobalType(const ASTModule &ASTCxt);

private:
  class ImportTypeContext;
  ImportTypeContext &Cxt;
  ImportType(ImportTypeContext &Cxt);
  ~ImportType() = default;

  friend class ASTModule;
};

class WASMEDGE_CPP_API_EXPORT ExportType {
public:
  ExternalType GetExternalType();
  std::string GetExternalName();
  const FunctionType GetFunctionType(const ASTModule &ASTCxt);
  const TableType GetTableType(const ASTModule &ASTCxt);
  const MemoryType GetMemoryType(const ASTModule &ASTCxt);
  const GlobalType GetGlobalType(const ASTModule &ASTCxt);

private:
  class ExportTypeContext;
  ExportTypeContext &Cxt;
  ExportType(ExportTypeContext &Cxt);
  ~ExportType() = default;

  friend class ASTModule;
};

// <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Async >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT Async {
  template <typename... Args> Async(Args &&... Val);
  ~Async() = default;
  class AsyncContext;
  std::unique_ptr<AsyncContext> Cxt;

public:
  void Wait();
  bool WaitFor(uint64_t Milliseconds);
  void Cancel();
  Result Get(std::vector<Value> &Returns);

  friend class VM;
};

// <<<<<<<< WasmEdge Async <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class WASMEDGE_CPP_API_EXPORT Configuration {
public:
  Configuration();
  ~Configuration() = default;

  // Methods for Proposals
  void AddProposal(const Proposal Prop);
  void RemoveProposal(const Proposal Prop);
  bool HasProposal(const Proposal Prop);

  // Methods for Host registration
  void AddHostRegistration(const HostRegistration Host);
  void RemoveHostRegistration(const HostRegistration Host);
  bool HasHostRegistration(const HostRegistration Host);

  // MaxMemoryPage
  void SetMaxMemoryPage(const uint32_t Page);
  uint32_t GetMaxMemoryPage();

  // Force Interpreter
  void SetForceInterpreter(const bool isForceInterpreter);
  bool IsForceInterpreter();

  // Compiler Config
  void CompilerSetOptimizationLevel(const CompilerOptimizationLevel Level);
  CompilerOptimizationLevel CompilerGetOptimizationLevel();
  void CompilerSetOutputFormat(const CompilerOutputFormat Format);
  CompilerOutputFormat CompilerGetOutputFormat();
  void CompilerSetDumpIR(const bool IsDump);
  bool CompilerIsDumpIR();
  void CompilerSetGenericBinary(const bool IsGeneric);
  bool CompilerIsGenericBinary();
  void CompilerSetInterruptible(const bool IsInterruptible);
  bool CompilerIsInterruptible();

  // Statistics Config
  void StatisticsSetInstructionCounting(const bool IsCount);
  bool StatisticsIsInstructionCounting();
  void StatisticsSetCostMeasuring(const bool IsMeasure);
  bool StatisticsIsCostMeasuring();
  void StatisticsSetTimeMeasuring(const bool IsMeasure);
  bool StatisticsIsTimeMeasuring();

private:
  class ConfigureContext;
  std::unique_ptr<ConfigureContext> Cxt;

  friend class Loader;
  friend class Validator;
  friend class Executor;
  friend class VM;
};

class WASMEDGE_CPP_API_EXPORT Statistics {
protected:
  Statistics() = default;
  ~Statistics() = default;

public:
  static Statistics New();
  static Statistics Move(Statistics &&StatCxt);

  uint64_t GetInstrCount();
  double GetInstrPerSecond();
  uint64_t GetTotalCost();

  void SetCostTable(std::vector<uint64_t> &CostArr);
  void SetCostLimit(const uint64_t Limit);
  void Clear();
};

// >>>>>>>> WasmEdge Runtime >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// >>>>>>>> WasmEdge Instances >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT FunctionInstance {
protected:
  FunctionInstance() = default;
  ~FunctionInstance() = default;

public:
  using HostFunc_t = std::function<Result(
      void *Data, const CallingFrame &CallFrameCxt,
      const std::vector<Value> &Params, std::vector<Value> &Returns)>;

  using WrapFunc_t = std::function<Result(
      void *This, void *Data, const CallingFrame &CallFrameCxt,
      const std::vector<Value> &Params, std::vector<Value> &Returns)>;

  static FunctionInstance New(const FunctionType &Type, HostFunc_t HostFunc,
                              void *Data, const uint64_t Cost);
  static FunctionInstance New(const FunctionType &Type, WrapFunc_t WrapFunc,
                              void *Binding, void *Data, const uint64_t Cost);

  const FunctionType GetFunctionType();
};

class WASMEDGE_CPP_API_EXPORT TableInstance {
protected:
  TableInstance() = default;
  ~TableInstance() = default;

public:
  static TableInstance New(const TableType &TabType);

  const TableType GetTableType();
  Result GetData(Value &Data, const uint32_t Offset);
  Result SetData(Value &Data, const uint32_t Offset);
  uint32_t GetSize();
  Result Grow(const uint32_t Size);
};

class WASMEDGE_CPP_API_EXPORT MemoryInstance {
protected:
  MemoryInstance() = default;
  ~MemoryInstance() = default;

public:
  static MemoryInstance New(const MemoryType &MemType);

  const MemoryType GetMemoryType();
  Result GetData(std::vector<uint8_t> &Data, const uint32_t Offset,
                 const uint32_t Length);

  Result SetData(const std::vector<uint8_t> &Data, const uint32_t Offset);

  std::vector<uint8_t> GetReference(const uint32_t Offset,
                                    const uint32_t Length);
  const std::vector<uint8_t> GetReferenceConst(const uint32_t Offset,
                                               const uint32_t Length);
  uint32_t GetPageSize();

  Result GrowPage(const uint32_t Page);
};

class WASMEDGE_CPP_API_EXPORT GlobalInstance {
protected:
  GlobalInstance() = default;
  ~GlobalInstance() = default;

public:
  static GlobalInstance New(const GlobalType &GlobType, const Value &Value);

  const GlobalType GetGlobalType();
  Value GetValue();

  void SetValue(const Value &Value);
};

class WASMEDGE_CPP_API_EXPORT ModuleInstance {
protected:
  ModuleInstance() = default;
  ~ModuleInstance() = default;

public:
  static ModuleInstance New();
  static ModuleInstance New(const std::string &ModuleName);
  static ModuleInstance New(const std::vector<const std::string> &Args,
                            const std::vector<const std::string> &Envs,
                            const std::vector<const std::string> &Preopens);
  static ModuleInstance Move(ModuleInstance &ModInst);

  void InitWASI(const std::vector<const std::string> &Args,
                const std::vector<const std::string> &Envs,
                const std::vector<const std::string> &Preopens);

  uint32_t WASIGetExitCode();
  uint32_t WASIGetNativeHandler(int32_t Fd, uint64_t &NativeHandler);

  void InitWasmEdgeProcess(const std::vector<const std::string> &AllowedCmds,
                           const bool AllowAll);
  std::string GetModuleName();

  FunctionInstance FindFunction(const std::string &Name);
  TableInstance FindTable(const std::string &Name);
  MemoryInstance FindMemory(const std::string &Name);
  GlobalInstance FindGlobal(const std::string &Name);

  std::vector<std::string> ListFunction();
  std::vector<std::string> ListTable();
  std::vector<std::string> ListMemory();
  std::vector<std::string> ListGlobal();

  void AddFunction(const std::string &Name, FunctionInstance &&FuncCxt);
  void AddTable(const std::string &Name, TableInstance &&TableCxt);
  void AddMemory(const std::string &Name, MemoryInstance &&MemoryCxt);
  void AddGlobal(const std::string &Name, GlobalInstance &&GlobalCxt);

  friend class VM;
  friend class Executor;
  friend class Store;
};

// <<<<<<<< WasmEdge Instances <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class WASMEDGE_CPP_API_EXPORT ASTModule {
protected:
  ASTModule() = default;
  ~ASTModule() = default;

public:
  static ASTModule New();
  static ASTModule Move(ASTModule &&Module);

  std::vector<ImportType> ListImports();
  std::vector<ExportType> ListExports();

  friend class ImportType;
  friend class ExportType;

  friend class VM;
  friend class Loader;
  friend class Validator;
  friend class Executor;
};

// >>>>>>>> WasmEdge Loader class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT Loader {
protected:
  Loader() = default;
  ~Loader() = default;

public:
  static Loader New(const Configuration &ConfCxt);
  static Loader Move(Loader &&LoadCxt);

  Result Parse(ASTModule &Module, const std::string &Path);
  Result Parse(ASTModule &Module, const std::vector<uint8_t> &Buf);
};

// <<<<<<<< WasmEdge Loader CLass <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Validator class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT Validator {
protected:
  Validator() = default;
  ~Validator() = default;

public:
  static Validator New(Configuration &ConfCxt);
  static Validator Move(Validator &&ValidCxt);

  Result Validate(const ASTModule &ASTCxt);
};

// <<<<<<<< WasmEdge Validator Class <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Executor class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT Executor {
protected:
  Executor() = default;
  ~Executor() = default;

public:
  static Executor New(const Configuration &ConfCxt, Statistics &StatCxt);
  static Executor Move(Executor &&ExecCxt);

  Result Instantiate(ModuleInstance &ModuleCxt, Store &StoreCxt,
                     const ASTModule &ASTCxt);

  Result Register(ModuleInstance &ModuleCxt, Store &StoreCxt,
                  const ASTModule &ASTCxt, const std::string &ModuleName);

  Result Register(Store &StoreCxt, const ModuleInstance &ImportCxt);

  Result Invoke(const FunctionInstance &FuncCxt,
                const std::vector<Value> &Params, std::vector<Value> &Returns);
};

// <<<<<<<< WasmEdge Executor Class <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class WASMEDGE_CPP_API_EXPORT Store {
protected:
  Store() = default;
  ~Store() = default;

public:
  static Store New();
  static Store Move(Store &&StoreCxt);

  const ModuleInstance FindModule(const std::string &Name);
  std::vector<std::string> ListModule();
};

class WASMEDGE_CPP_API_EXPORT CallingFrame {
protected:
  CallingFrame() = default;
  ~CallingFrame() = default;

public:
  Executor GetExecutor();
  const ModuleInstance GetModuleInstance();
  MemoryInstance GetMemoryInstance(const uint32_t Idx);
};

// <<<<<<<< WasmEdge Runtime <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge VM >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class WASMEDGE_CPP_API_EXPORT VM {
public:
  VM(const Configuration &ConfCxt, Store &StoreCxt);
  VM(const Configuration &ConfCxt);
  VM(Store &StoreCxt);
  VM();
  ~VM() = default;

  Result RegisterModule(const std::string &ModuleName, const std::string &Path);
  Result RegisterModule(const std::string &ModuleName,
                        const std::vector<uint8_t> &Buf);
  Result RegisterModule(const std::string &ModuleName, const ASTModule &ASTCxt);
  Result RegisterModule(const ModuleInstance &ImportCxt);

  Result RunWasm(const std::string &Path, const std::string &FuncName,
                 const std::vector<Value> &Params, std::vector<Value> &Returns);
  Result RunWasm(const std::vector<uint8_t> &Buf, const std::string &FuncName,
                 const std::vector<Value> &Params, std::vector<Value> &Returns);
  Result RunWasm(const ASTModule &ASTCxt, const std::string &FuncName,
                 const std::vector<Value> &Params, std::vector<Value> &Returns);

  std::unique_ptr<Async> AsyncRunWasm(const std::string &Path,
                                      const std::string &FuncName,
                                      const std::vector<Value> &Params);
  std::unique_ptr<Async> AsyncRunWasm(const std::vector<uint8_t> &Buf,
                                      const std::string &FuncName,
                                      const std::vector<Value> &Params);
  std::unique_ptr<Async> AsyncRunWasm(const ASTModule &ASTCxt,
                                      const std::string &FuncName,
                                      const std::vector<Value> &Params);

  Result LoadWasm(const std::string &Path);
  Result LoadWasm(const std::vector<uint8_t> &Buf);
  Result LoadWasm(const ASTModule &ASTCxt);

  Result Validate();
  Result Instantiate();

  Result Execute(const std::string &FuncName, const std::vector<Value> &Params,
                 std::vector<Value> &Returns);
  Result Execute(const std::string &ModuleName, const std::string &FuncName,
                 const std::vector<Value> &Params, std::vector<Value> &Returns);

  std::unique_ptr<Async> AsyncExecute(const std::string &FuncName,
                                      const std::vector<Value> &Params);
  std::unique_ptr<Async> AsyncExecute(const std::string &ModuleName,
                                      const std::string &FuncName,
                                      const std::vector<Value> &Params);

  const FunctionType GetFunctionType(const std::string &FuncName);
  const FunctionType GetFunctionType(const std::string &ModuleName,
                                     const std::string &FuncName);

  void Cleanup();
  uint32_t GetFunctionList(std::vector<std::string> &Names,
                           std::vector<FunctionType> &FuncTypes);

  ModuleInstance GetImportModuleContext(const HostRegistration Reg);
  const ModuleInstance GetActiveModule();
  const ModuleInstance GetRegisteredModule(const std::string &ModuleName);

  std::vector<std::string> ListRegisteredModule();

  Store GetStoreContext();
  Loader GetLoaderContext();
  Validator GetValidatorContext();
  Executor GetExecutorContext();
  Statistics GetStatisticsContext();

private:
  class VMContext;
  std::unique_ptr<VMContext> Cxt;
};

// <<<<<<<< WasmEdge VM <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
} // namespace SDK
} // namespace WasmEdge

#endif // WASMEDGE_CPP_API_HH