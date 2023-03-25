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
    ~FunctionType();

    uint32_t GetParametersLength();
    std::vector<ValType> GetParameters();

    uint32_t GetReturnsLength();
    std::vector<ValType> GetReturns();
  private:
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT ImportType {
    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT ExportType {
    // TODO
  };

  // <<<<<<<< WasmEdge Data Structures <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

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

  // >>>>>>>> WasmEdge Loader class >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT Loader {
  public:
    Loader(const ConfigureContext &ConfCxt);
    ~Loader();

    Result ParseFromFile(std::unique_ptr<ASTModule> Module, const std::string Path);
    Result ParseFromBuffer(std::unique_ptr<ASTModule> Module,
                          const std::vector<uint8_t> Buf, const uint32_t BufLen);

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

  class WASMEDGE_CPP_API_EXPORT Store {
  public:
    Store();
    ~Store() = default;

    const ModuleInstance &FindModule(const String Name);
    uint32_t ListModuleLength();
    uint32_t ListModule(std::vector<String> &Names, const uint32_t Len);

  private:
    // StoreContext StoreCxt; // TODO
  };

  // >>>>>>>> WasmEdge Instances >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  class WASMEDGE_CPP_API_EXPORT ModuleInstance {
  public:
    ModuleInstance(const String ModuleName);
    ModuleInstance(const std::vector<const std::string> Args,
                  const uint32_t ArgLen,
                  const std::vector<const std::string> Envs,
                  const uint32_t EnvLen,
                  const std::vector<const std::string> Preopens,
                  const uint32_t PreopenLen);
    ~ModuleInstance();

    // TODO
  };

  class WASMEDGE_CPP_API_EXPORT FunctionInstance {
  public:
    // TODO
  };



  // <<<<<<<< WasmEdge Instances <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  class WASMEDGE_CPP_API_EXPORT ASTModule {
  public:
    ASTModule();
    ~ASTModule();

    uint32_t ListImportsLength();
    uint32_t ListImports(const std::vector<ImportType> &Imports,
                        const uint32_t Len);

    uint32_t ListExportsLength();
    uint32_t ListExports(const std::vector<ExportType> &Exports,
                        const uint32_t Len);
  private:
    // TODO
  };  
}

#endif // WASMEDGE_CPP_API_HH