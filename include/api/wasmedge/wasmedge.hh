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

#include "string"
#include "vector"
#include "memory"
#include "functional"

namespace WasmEdge {
  enum WASMEDGE_CPP_API_EXPORT ErrCategory {
    WASM = 0x00,
    UserLevelError = 0x01
  };

  enum WASMEDGE_CPP_API_EXPORT HostRegistration {
    Wasi,
    WasmEdge_Process,
    WasiNN,
    WasiCrypto_Common,
    WasiCrypto_AsymmetricCommon,
    WasiCrypto_Kx,
    WasiCrypto_Signatures,
    WasiCrypto_Symmetric
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

  // TODO: should Version get its own namespace?
  WASMEDGE_CPP_API_EXPORT std::string VersionGet();
  WASMEDGE_CPP_API_EXPORT uint32_t VersionGetMajor();
  WASMEDGE_CPP_API_EXPORT uint32_t VersionGetMinor();
  WASMEDGE_CPP_API_EXPORT uint32_t VersionGetPatch();

  // TODO: Should Log get its own namespace?
  WASMEDGE_CPP_API_EXPORT void LogSetErrorLevel();
  WASMEDGE_CPP_API_EXPORT void LogSetDebugLevel();
  WASMEDGE_CPP_API_EXPORT void LogOff();

  class WASMEDGE_CPP_API_EXPORT Value {
  public:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT String {
  public:
    String(const std::string Str);
    String(const std::string Buf, const uint32_t Len);
    ~String() = default;
    static String Wrap(const std::string Buf, const uint32_t Len);

    bool IsEqual(const String Str);
    uint32_t Copy(std::string Buf, const uint32_t Len);

  private:
    uint32_t Len;
    const std::string Buf;
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
    Limit(bool HasMax, bool Shared, uint32_t Min, uint32_t Max)
    : HasMax(HasMax),
      Shared(Shared),
      Min(Min),
      Max(Max) {};
    ~Limit() = default;

    bool HasMax { false };
    bool Shared { false };
    uint32_t Min { 0 };
    uint32_t Max { 0 };

    bool IsEqual(const Limit Lim);
  };

  class WASMEDGE_CPP_API_EXPORT FunctionType {
  public:
    FunctionType(const std::vector<ValType> &ParamList,
                const uint32_t ParamLen,
                const std::vector<ValType> &ReturnList,
                const uint32_t ReturnLen);
    ~FunctionType() = default;

    uint32_t GetParametersLength();
    std::vector<ValType> GetParameters();

    uint32_t GetReturnsLength();
    std::vector<ValType> GetReturns();
  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT TableType {
  public:
    TableType(const RefType RefType, const Limit Limit);
    ~TableType() = default;

    RefType GetRefType();
    Limit GetLimit();
  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT MemoryType {
  public:
    MemoryType(const Limit Limit);
    ~MemoryType() = default;

    Limit GetLimit();

  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT GlobalType {
  public:
    GlobalType(const ValType, const Mutability Mut);
    ~GlobalType() = default;

    ValType GetValType();
    Mutability GetMutability();
  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT ImportType {
  public:
    // TODO: constructor

    ExternalType GetExternalType();
    String GetModuleName();
    String GetExternalName();
    const FunctionType &GetFunctionType(const ASTModule &ASTCxt);
    const TableType &GetTableType(const ASTModule &ASTCxt);
    const MemoryType &GetMemoryType(const ASTModule &ASTCxt);
    const GlobalType &GetGlobalType(const ASTModule &ASTCxt);

  private:
    // TODO
  };

  // TODO: ImportType & ExportType both look the same
  // Create a common interface/parent class then?
  class WASMEDGE_CPP_API_EXPORT ExportType {
  public:
    // TODO: constructor

    ExternalType GetExternalType();
    String GetExternalName();
    const FunctionType &GetFunctionType(const ASTModule &ASTCxt);
    const TableType &GetTableType(const ASTModule &ASTCxt);
    const MemoryType &GetMemoryType(const ASTModule &ASTCxt);
    const GlobalType &GetGlobalType(const ASTModule &ASTCxt);
  };

  // <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // >>>>>>>> WasmEdge Async >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  
  class WASMEDGE_CPP_API_EXPORT Async {
    Async();
    ~Async() = default;

    void Wait();
    bool WaitFor(uint64_t Milliseconds);
    void Cancel();
    uint32_t GetReturnsLength();
    Result Get(std::vector<Value> &Returns, const uint32_t ReturnLen);
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

    void SetCostTable(std::vector<uint64_t> CostArr, const uint32_t Len);
    void SetCostLimit(const uint64_t Limit);
    void Clear();

  private:
    // TODO
  };

  // >>>>>>>> WasmEdge Runtime >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT VM {
  public:
    VM(const ConfigureContext &ConfCxt, Store &StoreCxt);
    ~VM() = default;

    Result RegisterModuleFromFile(const String ModuleName,
                                    const std::string &Path);
    Result RegisterModuleFromBuffer(const String ModuleName,
                  const std::vector<uint8_t> &Buf, const uint32_t BufLen);
    Result RegisterModuleFromASTModule(const String ModuleName,
                                      const ASTModule &ASTCxt);
    Result RegisterModuleFromImport(const ModuleInstance &ImportCxt);

    Result RunWasmFromFile(const std::string &Path, const String FuncName,
                  const std::vector<Value> &Params, const uint32_t ParamLen,
                  std::vector<Value> &Returns, const uint32_t ReturnLen);
    Result RunWasmFromBuffer(const std::vector<uint8_t> &Buf,
                  const uint32_t BufLen, const String FuncName,
                  const std::vector<Value> &Params, const uint32_t ParamLen,
                  std::vector<Value> &Returns, const uint32_t ReturnLen);
    Result RunWasmFromASTModule(const ASTModule &ASTCxt, const String FuncName,
                  const std::vector<Value> &Params, const uint32_t ParamLen,
                  std::vector<Value> &Returns, const uint32_t ReturnLen);

    std::unique_ptr<Async> AsyncRunWasmFromFile(const std::string &Path,
                  const String FuncName, const std::vector<Value> &Params,
                  const uint32_t ParamLen);
    std::unique_ptr<Async> AsyncRunWasmFromBuffer(
                  const std::vector<uint8_t> &Buf, const uint32_t BufLen,
                  const String FuncName, const std::vector<Value> &Params,
                  const uint32_t ParamLen);
    std::unique_ptr<Async> AsyncRunWasmFromASTModule(const ASTModule &ASTCxt,
                  const String FuncName, const std::vector<Value> &Params,
                  const uint32_t ParamLen);

    Result LoadWasmFromFile(const std::string &Path);
    Result LoadWasmFromBuffer(const std::vector<uint8_t> &Buf,
                  const uint32_t BufLen);
    Result LoadWasmFromASTModule(const ASTModule &ASTCxt);

    Result Validate();
    Result Instantiate();

    Result Execute(const String FuncName, const std::vector<Value> &Params,
                  const uint32_t ParamLen, const std::vector<Value> &Returns,
                  const uint32_t ReturnLen);
    Result ExecuteRegistered(const String ModuleName, const String FuncName,
                  const std::vector<Value> &Params, const uint32_t ParamLen,
                  std::vector<Value> &Returns, const uint32_t ReturnLen);

    std::unique_ptr<Async> AsyncExecute(const String FuncName,
                  const std::vector<Value> &Params, const uint32_t ParamLen);
    std::unique_ptr<Async> AsyncExecuteRegistered(const String ModuleName,
                  const String FuncName, const std::vector<Value> &Params,
                  const uint32_t ParamLen);

    std::unique_ptr<const FunctionType> GetFunctionType(const String FuncName);
    std::unique_ptr<const FunctionType> GetFunctionTypeRegistered(
                  const String ModuleName, const String FuncName);

    void Cleanup();
    uint32_t GetFunctionListLength();
    uint32_t GetFunctionList(std::vector<String> &Names,
                  const std::vector<FunctionType &> &FuncTypes,
                  const uint32_t Len);

    std::unique_ptr<ModuleInstance> GetImportModuleContext(
                  const HostRegistration Reg);
    std::unique_ptr<const ModuleInstance> GetActiveModule();
    std::unique_ptr<const ModuleInstance> GetRegisteredModule(
                  const String ModuleName);

    uint32_t ListRegisteredModuleLength();
    uint32_t ListRegisteredModule(std::vector<String> &Names,
                                  const uint32_t Len);

    std::unique_ptr<Store> GetStoreContext();
    std::unique_ptr<Loader> GetLoaderContext();
    std::unique_ptr<Validator> GetValidatorContext();
    std::unique_ptr<Executor> GetExecutorContext();
    std::unique_ptr<StatisticsContext> GetStatisticsContext();
  private:
    // TODO
  };

  // <<<<<<<< WasmEdge VM <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // >>>>>>>> WasmEdge Runtime >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  // >>>>>>>> WasmEdge Loader class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Loader {
  public:
    Loader(const ConfigureContext &ConfCxt);
    ~Loader() = default;

    Result ParseFromFile(std::unique_ptr<ASTModule> Module,
                        const std::string Path);
    Result ParseFromBuffer(std::unique_ptr<ASTModule> Module,
                          const std::vector<uint8_t> Buf,
                          const uint32_t BufLen);

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
              StatisticsContext & StatCxt);
    ~Executor() = default;

    Result Instantiate(std::unique_ptr<ModuleInstance> ModuleCxt,
                      Store &StoreCxt,
                      const ASTModule &ASTCxt);

    Result Register(std::unique_ptr<ModuleInstance> ModuleCxt,
                    Store &StoreCxt,
                    const ASTModule &ASTCxt,
                    String ModuleName);

    Result RegisterImport(Store &StoreCxt,
                          const ModuleInstance &ImportCxt);

    Result Invoke(const FunctionInstance &FuncCxt,
                  const std::vector<String> &Params, const uint32_t ParamLen,
                  std::vector<Value> &Returns, const uint32_t ReturnLen);

  private:
    // ExecutorContext ExecutorCxt; // TODO
  };

  // <<<<<<<< WasmEdge Executor Class <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  class WASMEDGE_CPP_API_EXPORT ASTModule {
  public:
    ASTModule();
    ~ASTModule() = default;

    uint32_t ListImportsLength();
    uint32_t ListImports(const std::vector<ImportType> &Imports,
                        const uint32_t Len);

    uint32_t ListExportsLength();
    uint32_t ListExports(const std::vector<ExportType> &Exports,
                        const uint32_t Len);
  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT Store {
  public:
    Store();
    ~Store() = default;

    std::unique_ptr<const ModuleInstance> FindModule(const String Name);
    uint32_t ListModuleLength();
    uint32_t ListModule(std::vector<String> &Names, const uint32_t Len);

  private:
    // StoreContext StoreCxt; // TODO
  };

  // >>>>>>>> WasmEdge Instances >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT ModuleInstance {
  public:
    ModuleInstance(const String ModuleName);
    ModuleInstance(const std::vector<const std::string> &Args,
                  const uint32_t ArgLen,
                  const std::vector<const std::string> &Envs,
                  const uint32_t EnvLen,
                  const std::vector<const std::string> &Preopens,
                  const uint32_t PreopenLen);
    ~ModuleInstance() = default;

    void InitWASI(const std::vector<const std::string> &Args,
                  const uint32_t ArgLen,
                  const std::vector<const std::string> &Envs,
                  const uint32_t EnvLen,
                  const std::vector<const std::string> &Preopens,
                  const uint32_t PreopenLen);

    uint32_t WASIGetExitCode();
    uint32_t WASIGetNativeHandler(int32_t Fd, uint64_t &NativeHandler);

    void InitWasmEdgeProcess(const std::vector<const std::string> &AllowedCmds,
                            const uint32_t CmdsLen,
                            const bool AllowAll);
    String GetModuleName();

    FunctionInstance &FindFunction(const String Name);
    TableInstance &FindTable(const String Name);
    MemoryInstance &FindMemory(const String Name);
    GlobalInstance &FindGlobal(const String Name);

    uint32_t ListFunctionLength();
    uint32_t ListFunction(std::vector<std::string> &Names, const uint32_t Len);

    uint32_t ListTableLenth();
    uint32_t ListTable(std::vector<std::string> &Names, const uint32_t Len);

    uint32_t ListMemoryLength();
    uint32_t ListMemory(std::vector<std::string> &Names, const uint32_t Len);

    uint32_t ListGlobalLength();
    uint32_t ListGlobal(std::vector<std::string> &Names, const uint32_t Len);

    void AddFunction(const String Name,
                    FunctionInstance &FuncCxt);
    void AddTable(const String Name,
                  TableInstance &TableCxt);
    void AddMemory(const String Name,
                  MemoryInstance &MemoryCxt);
    void AddGlobal(const String Name,
                  GlobalInstance GlobalCxt);

  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT FunctionInstance {
  public:
    using HostFunc_t = std::function<Result(
      std::unique_ptr<void> Data, const CallingFrame &CallFrameCxt,
      const std::vector<Value> &Params, std::vector<Value> &Returns)>;

    using WrapFunc_t = std::function<Result(
      std::unique_ptr<void> This, std::unique_ptr<void> Data,
      const CallingFrame &CallFrameCxt, const std::vector<Value> &Params,
      const uint32_t ParamLen, std::vector<Value> &Returns,
      const uint32_t ReturnLen)>;

    FunctionInstance(const FunctionType &Type,
                    HostFunc_t HostFunc, std::unique_ptr<void> Data,
                    const uint64_t Cost);
    FunctionInstance(const FunctionType &Type,
                    WrapFunc_t WrapFunc,
                    std::unique_ptr<void> Binding,
                    std::unique_ptr<void> Data,
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
    Result GetData(std::vector<Value> &Data, const uint32_t Offset);
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
    Result GetData(std::vector<uint8_t> &Data, const uint32_t Offset,
                  const uint32_t Length);

    Result SetData(const std::vector<uint8_t> &Data,
                   const uint32_t Offset, const uint32_t Length);

    std::vector<uint8_t> &GetReference(const uint32_t Offset,
                                      const uint32_t Length);
    const std::vector<uint8_t> &GetReferenceConst(const uint32_t Offset,
                                                  const uint32_t Length);
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
    std::unique_ptr<Executor> GetExecutor();
    std::unique_ptr<const ModuleInstance> GetModuleInstance();
    std::unique_ptr<MemoryInstance> GetMemoryInstance(const uint32_t Idx);
  };

  // <<<<<<<< WasmEdge Runtime <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}

#endif // WASMEDGE_CPP_API_HH