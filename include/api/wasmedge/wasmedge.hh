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

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "wasmedge/int128.h"
#include "wasmedge/version.h"

namespace WasmEdge {
  enum WASMEDGE_CPP_API_EXPORT ErrCategory {
    WASM = 0x00,
    UserLevelError = 0x01
  };

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

  enum WASMEDGE_CPP_API_EXPORT Mutability {
    Const = 0x00,
    Var = 0x01
  };

  // TODO: ValType is an extension of RefType
  enum WASMEDGE_CPP_API_EXPORT RefType {
    FuncRef = 0x70,
    ExternRef = 0x6F
  };

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
    const FunctionInstance &GetFuncRef();
    std::shared_ptr<void> GetExternRef();

    bool IsNullRef();

  private:
    uint128_t Val;
    ValType Type;
  };

  class WASMEDGE_CPP_API_EXPORT Result {
  public:
    Result(const ErrCategory Category, const uint32_t Code);
    ~Result() = default;

    // static methods
    static Result Success() { return Result{ 0x00 }; }
    static Result Terminate() { return Result{ 0x01 }; }
    static Result Fail() { return Result{ 0x02 }; }

    bool IsOk();
    uint32_t GetCode();
    ErrCategory GetCategory();
    const std::string GetMessage();

  private:
    uint32_t Code;
    Result(const uint32_t Code);
  };

  // >>>>>>>> WasmEdge Data Structures >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Limit {
  public:
    Limit(bool HasMax = false, bool Shared = false, uint32_t Min = 0, uint32_t Max = 0)
    : HasMax(HasMax),
      Shared(Shared),
      Min(Min),
      Max(Max) {};
    ~Limit() = default;

    bool HasMax;
    bool Shared;
    uint32_t Min;
    uint32_t Max;

    bool operator==(const Limit &Lim);
  };

  class WASMEDGE_CPP_API_EXPORT FunctionType {
  public:
    FunctionType(const std::vector<ValType> &ParamList,
                 const std::vector<ValType> &ReturnList);
    ~FunctionType() = default;

    const std::vector<ValType> &GetParameters();
    const std::vector<ValType> &GetReturns();
  private:
    class FunctionTypeContext;
    std::unique_ptr<FunctionTypeContext> Cxt;

    friend class ImportType;
    friend class ExportType;
    FunctionType() = default;
  };

  class WASMEDGE_CPP_API_EXPORT TableType {
  public:
    TableType(const RefType RefType, const Limit &Limit);
    ~TableType() = default;

    RefType GetRefType();
    const Limit &GetLimit();
  private:
    class TableTypeContext;
    std::unique_ptr<TableTypeContext> Cxt;

    friend class ImportType;
    friend class ExportType;
    TableType() = default;
  };

  class WASMEDGE_CPP_API_EXPORT MemoryType {
  public:
    MemoryType(const Limit &Limit);
    ~MemoryType() = default;

    const Limit &GetLimit();

  private:
    class MemoryTypeContext;
    std::unique_ptr<MemoryTypeContext> Cxt;

    friend class ImportType;
    friend class ExportType;
    MemoryType() = default;
  };

  class WASMEDGE_CPP_API_EXPORT GlobalType {
  public:
    GlobalType(const ValType, const Mutability Mut);
    ~GlobalType() = default;

    ValType GetValType();
    Mutability GetMutability();
  private:
    class GlobalTypeContext;
    std::unique_ptr<GlobalTypeContext> Cxt;

    friend class ImportType;
    friend class ExportType;
    GlobalType() = default;
  };

  class WASMEDGE_CPP_API_EXPORT ImportType {
  public:

    ExternalType GetExternalType();
    std::string GetModuleName();
    std::string GetExternalName();
    std::shared_ptr<const FunctionType> GetFunctionType(const ASTModule &ASTCxt);
    std::shared_ptr<const TableType> GetTableType(const ASTModule &ASTCxt);
    std::shared_ptr<const MemoryType> GetMemoryType(const ASTModule &ASTCxt);
    std::shared_ptr<const GlobalType> GetGlobalType(const ASTModule &ASTCxt);

  private:
    ImportType();
    ~ImportType() = default;
    class ImportTypeContext;
    std::unique_ptr<ImportTypeContext> Cxt;

    friend class ASTModule;
  };

  class WASMEDGE_CPP_API_EXPORT ExportType {
  public:

    ExternalType GetExternalType();
    std::string GetExternalName();
    std::shared_ptr<const FunctionType> GetFunctionType(const ASTModule &ASTCxt);
    std::shared_ptr<const TableType> GetTableType(const ASTModule &ASTCxt);
    std::shared_ptr<const MemoryType> GetMemoryType(const ASTModule &ASTCxt);
    std::shared_ptr<const GlobalType> GetGlobalType(const ASTModule &ASTCxt);

  private:
    ExportType();
    ~ExportType() = default;
    class ExportTypeContext;
    std::unique_ptr<ExportTypeContext> Cxt;

    friend class ASTModule;
  };

  // <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // >>>>>>>> WasmEdge Async >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Async {
    Async();
    ~Async() = default;

  public:
    void Wait();
    bool WaitFor(uint64_t Milliseconds);
    void Cancel();
    Result Get(std::vector<Value> &Returns);

    friend class VM;
  };

  // <<<<<<<< WasmEdge Async <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  class WASMEDGE_CPP_API_EXPORT ConfigureContext {
  public:
    ConfigureContext();
    ~ConfigureContext() = default;

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

    // Force Intepreter
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
    // TODO: Break into three Configs and declare them in this namespace?
    // CompilerConfig CompConfig;
    // StatisticsConfig StatisticsConf;
    // RuntimeConfig RuntimeConfig;

    // TODO: Or use the WasmEdge::Configure internal API somehow?
    // And call Configure's methods internally in its methods?
    // Because Configure already has the required methods
    // For example -
    // In the AddProposal method -
    // void AddProposal(const Proposal Prop)
    // {
    //   this.Conf.addProposal(Prop);
    // }
    // Configure Conf;
  };

  class WASMEDGE_CPP_API_EXPORT StatisticsContext {
  public:
    StatisticsContext();
    ~StatisticsContext() = default;

    uint64_t GetInstrCount();
    double GetInstrPerSecond();
    uint64_t GetTotalCost();

    void SetCostTable(std::vector<uint64_t> &CostArr);
    void SetCostLimit(const uint64_t Limit);
    void Clear();

  private:
    // TODO
  };

  // >>>>>>>> WasmEdge Runtime >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  // >>>>>>>> WasmEdge Loader class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Loader {
  public:
    Loader(const ConfigureContext &ConfCxt);
    ~Loader() = default;

    Result Parse(ASTModule &Module,
                        const std::string &Path);
    Result Parse(ASTModule &Module,
                        const std::vector<uint8_t> &Buf);

  private:
    // LoaderContext LoaderCxt; // TODO
  };

  // <<<<<<<< WasmEdge Loader CLass <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // >>>>>>>> WasmEdge Validator class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Validator {
  public:
    Validator(ConfigureContext &ConfCxt);
    ~Validator() = default;

    Result Validate(const ASTModule &ASTCxt);
  private:
    // ValidatorContext ValidatorCxt; // TODO
  };

  // <<<<<<<< WasmEdge Validator Class <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // >>>>>>>> WasmEdge Executor class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Executor {
  public:
    Executor(const ConfigureContext &ConfCxt,
             StatisticsContext &StatCxt);
    ~Executor() = default;

    Result Instantiate(ModuleInstance &ModuleCxt,
                       Store &StoreCxt,
                       const ASTModule &ASTCxt);

    Result Register(ModuleInstance &ModuleCxt,
                    Store &StoreCxt,
                    const ASTModule &ASTCxt,
                    const std::string &ModuleName);

    Result Register(Store &StoreCxt,
                    const ModuleInstance &ImportCxt);

    Result Invoke(const FunctionInstance &FuncCxt,
                  const std::vector<std::string> &Params,
                  std::vector<Value> &Returns);

  private:
    // ExecutorContext ExecutorCxt; // TODO
  };

  // <<<<<<<< WasmEdge Executor Class <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  class WASMEDGE_CPP_API_EXPORT ASTModule {
  public:
    ASTModule();
    ~ASTModule() = default;

    std::vector<ImportType> ListImports();
    std::vector<ExportType> ListExports();

    friend class ImportType;
    friend class ExportType;
  private:
    class ASTModuleContext;
    std::unique_ptr<ASTModuleContext> Cxt;
  };

  class WASMEDGE_CPP_API_EXPORT Store {
  public:
    Store();
    ~Store() = default;

    const ModuleInstance &FindModule(const std::string &Name);
    std::vector<std::string> ListModule();

  private:
    // StoreContext StoreCxt; // TODO
  };

  // >>>>>>>> WasmEdge Instances >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT ModuleInstance {
  public:
    ModuleInstance();
    ModuleInstance(const std::string &ModuleName);
    ModuleInstance(const std::vector<const std::string> &Args,
                   const std::vector<const std::string> &Envs,
                   const std::vector<const std::string> &Preopens);
    ~ModuleInstance() = default;

    void InitWASI(const std::vector<const std::string> &Args,
                  const std::vector<const std::string> &Envs,
                  const std::vector<const std::string> &Preopens);

    uint32_t WASIGetExitCode();
    uint32_t WASIGetNativeHandler(int32_t Fd, uint64_t &NativeHandler);

    void InitWasmEdgeProcess(const std::vector<const std::string> &AllowedCmds,
                            const bool AllowAll);
    std::string GetModuleName();

    FunctionInstance &FindFunction(const std::string &Name);
    TableInstance &FindTable(const std::string &Name);
    MemoryInstance &FindMemory(const std::string &Name);
    GlobalInstance &FindGlobal(const std::string &Name);

    std::vector<std::string> ListFunction();
    std::vector<std::string> ListTable();
    std::vector<std::string> ListMemory();
    std::vector<std::string> ListGlobal();

    void AddFunction(const std::string &Name,
                     FunctionInstance &&FuncCxt);
    void AddTable(const std::string &Name,
                  TableInstance &&TableCxt);
    void AddMemory(const std::string &Name,
                   MemoryInstance &&MemoryCxt);
    void AddGlobal(const std::string &Name,
                   GlobalInstance &&GlobalCxt);

  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT FunctionInstance {
  public:
    using HostFunc_t = std::function<Result(
      std::shared_ptr<void> Data, const CallingFrame &CallFrameCxt,
      const std::vector<Value> &Params, std::vector<Value> &Returns)>;

    using WrapFunc_t = std::function<Result(
      std::shared_ptr<void> This, std::shared_ptr<void> Data,
      const CallingFrame &CallFrameCxt, const std::vector<Value> &Params,
      std::vector<Value> &Returns)>;

    FunctionInstance(const FunctionType &Type,
                    HostFunc_t HostFunc, std::shared_ptr<void> Data,
                    const uint64_t Cost);
    FunctionInstance(const FunctionType &Type,
                    WrapFunc_t WrapFunc,
                    std::shared_ptr<void> Binding,
                    std::shared_ptr<void> Data,
                    const uint64_t Cost);
    ~FunctionInstance() = default;

    const FunctionType &GetFunctionType();
  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT TableInstance {
  public:
    TableInstance(const TableType &TabType);
    ~TableInstance();

    const TableType &GetTableType();
    Result GetData(Value &Data, const uint32_t Offset);
    Result SetData(Value Data, const uint32_t Offset);
    uint32_t GetSize();
    Result Grow(const uint32_t Size);

  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT MemoryInstance {
  public:
    MemoryInstance(const MemoryType &MemType);
    ~MemoryInstance() = default;

    const MemoryType &GetMemoryType();
    Result GetData(std::vector<uint8_t> &Data, const uint32_t Offset);

    Result SetData(const std::vector<uint8_t> &Data,
                   const uint32_t Offset);

    std::vector<uint8_t> &GetReference(const uint32_t Offset);
    const std::vector<uint8_t> &GetReferenceConst(const uint32_t Offset);
    uint32_t GetPageSize();

    Result GrowPage(const uint32_t Page);

  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT GlobalInstance {
  public:
    GlobalInstance(const GlobalType &GlobType, const Value Value);
    ~GlobalInstance() = default;

    const GlobalType &GetGlobalType();
    Value GetValue();

    void SetValue(const Value Value);
  private:
    // TODO
  };
  // <<<<<<<< WasmEdge Instances <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  class WASMEDGE_CPP_API_EXPORT CallingFrame {
    CallingFrame();
    ~CallingFrame() = default;

  public:
    Executor &GetExecutor();
    const ModuleInstance &GetModuleInstance();
    MemoryInstance &GetMemoryInstance(const uint32_t Idx);
  };

  // <<<<<<<< WasmEdge Runtime <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // >>>>>>>> WasmEdge VM >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT VM {
  public:
    VM(const ConfigureContext &ConfCxt, Store &StoreCxt);
    ~VM() = default;

    Result RegisterModule(const std::string &ModuleName,
                                  const std::string &Path);
    Result RegisterModule(const std::string &ModuleName,
                                    const std::vector<uint8_t> &Buf);
    Result RegisterModule(const std::string &ModuleName,
                                       const ASTModule &ASTCxt);
    Result RegisterModule(const ModuleInstance &ImportCxt);

    Result RunWasm(const std::string &Path, const std::string &FuncName,
                           const std::vector<Value> &Params,
                           std::vector<Value> &Returns);
    Result RunWasm(const std::vector<uint8_t> &Buf,
                  const std::string &FuncName, const std::vector<Value> &Params,
                  std::vector<Value> &Returns);
    Result RunWasm(const ASTModule &ASTCxt,
                  const std::string &FuncName, const std::vector<Value> &Params,
                  std::vector<Value> &Returns);

    std::unique_ptr<Async> AsyncRunWasm(const std::string &Path,
                  const std::string &FuncName, const std::vector<Value> &Params);
    std::unique_ptr<Async> AsyncRunWasm(
                  const std::vector<uint8_t> &Buf, const std::string &FuncName,
                  const std::vector<Value> &Params);
    std::unique_ptr<Async> AsyncRunWasm(const ASTModule &ASTCxt,
                  const std::string &FuncName, const std::vector<Value> &Params);

    Result LoadWasm(const std::string &Path);
    Result LoadWasm(const std::vector<uint8_t> &Buf);
    Result LoadWasm(const ASTModule &ASTCxt);

    Result Validate();
    Result Instantiate();

    Result Execute(const std::string &FuncName, const std::vector<Value> &Params,
                  std::vector<Value> &Returns);
    Result Execute(const std::string &ModuleName,
                  const std::string &FuncName, const std::vector<Value> &Params,
                  std::vector<Value> &Returns);

    std::unique_ptr<Async> AsyncExecute(const std::string &FuncName,
                  const std::vector<Value> &Params);
    std::unique_ptr<Async> AsyncExecute(const std::string &ModuleName,
                  const std::string &FuncName, const std::vector<Value> &Params);

    const FunctionType &GetFunctionType(const std::string &FuncName);
    const FunctionType &GetFunctionType(const std::string &ModuleName,
                                        const std::string &FuncName);

    void Cleanup();
    uint32_t GetFunctionList(std::vector<std::string> &Names,
                  std::vector<const FunctionType> &FuncTypes);

    ModuleInstance &GetImportModuleContext(const HostRegistration Reg);
    const ModuleInstance& GetActiveModule();
    const ModuleInstance &GetRegisteredModule(const std::string &ModuleName);

    std::vector<std::string> ListRegisteredModule();

    Store &GetStoreContext();
    Loader &GetLoaderContext();
    Validator &GetValidatorContext();
    Executor &GetExecutorContext();
    StatisticsContext &GetStatisticsContext();
  private:
    // TODO
  };

  // <<<<<<<< WasmEdge VM <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}

#endif // WASMEDGE_CPP_API_HH