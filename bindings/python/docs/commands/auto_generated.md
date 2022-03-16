```
Help on module WasmEdge:

NAME
    WasmEdge

DESCRIPTION
    WasmEdge
    -----------------------
    .. currentmodule:: WasmEdge
    .. autosummary::
       :toctree: _generate
       
       VersionGet
       VersionGetMajor
       VersionGetMinor
       VersionGetPatch
       Logging
       Configure
       Statistics
       Optimization
       Type
       ExternalType
       RefType
       Store
       Value
       CompilerOutput
       Result
       Proposal
       Host
       ASTModule
       Loader
       Validator
       Executor
       VM
       FunctionType
       Function
       ImportType
       ImportObject
       Limit
       MemoryType
       Mutability
       GlobalType
       Memory
       TableType
       Table

CLASSES
    pybind11_builtins.pybind11_object(builtins.object)
        ASTModule
        Async
        Compiler
        CompilerOutput
        Configure
        Executor
        ExportType
        ExternalType
        Function
        FunctionType
        Global
        GlobalType
        Host
        ImportObject
        ImportType
        Limit
        Loader
        Logging
        Memory
        MemoryType
        Mutability
        Optimization
        Proposal
        RefType
        Result
        Statistics
        Store
        Table
        TableType
        Type
        VM
        Validator
        Value
    
    class ASTModule(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      ASTModule
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  ListExports(...)
     |      ListExports(self: WasmEdge.ASTModule, arg0: int) -> list
     |  
     |  ListExportsLength(...)
     |      ListExportsLength(self: WasmEdge.ASTModule) -> int
     |  
     |  ListImports(...)
     |      ListImports(self: WasmEdge.ASTModule, arg0: int) -> list
     |  
     |  ListImportsLength(...)
     |      ListImportsLength(self: WasmEdge.ASTModule) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.ASTModule) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Async(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Async
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  Cancel(...)
     |      Cancel(self: WasmEdge.Async) -> None
     |  
     |  Get(...)
     |      Get(self: WasmEdge.Async, arg0: int) -> tuple
     |  
     |  GetReturnsLength(...)
     |      GetReturnsLength(self: WasmEdge.Async) -> int
     |  
     |  Wait(...)
     |      Wait(self: WasmEdge.Async) -> None
     |  
     |  WaitFor(...)
     |      WaitFor(self: WasmEdge.Async, arg0: int) -> bool
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Async) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Compiler(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Compiler
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  Compile(...)
     |      Compile(self: WasmEdge.Compiler, arg0: str, arg1: str) -> WasmEdge.Result
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Compiler, arg0: WasmEdge.Configure) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class CompilerOutput(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  Native
     |  
     |  Wasm
     |  
     |  Method resolution order:
     |      CompilerOutput
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.CompilerOutput) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.CompilerOutput, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.CompilerOutput) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.CompilerOutput, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  Native = <CompilerOutput.Native: 0>
     |  
     |  Wasm = <CompilerOutput.Wasm: 1>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Configure(pybind11_builtins.pybind11_object)
     |  The configuration context, `WasmEdge_ConfigureContext`, 
     |  is wrapped in the class `Configure` manages the 
     |  configurations for `Loader`, `Validator`, `Executor`,
     |  `VM`, and `Compiler`. Developers can adjust the settings
     |  about the proposals, VM host pre-registrations (such as 
     |  `WASI`), and AOT compiler options, and then apply the
     |  `Configure` context to create other runtime contexts.
     |  
     |  Method resolution order:
     |      Configure
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  AddHostRegistration(...)
     |      AddHostRegistration(self: WasmEdge.Configure, arg0: WasmEdge_HostRegistration) -> None
     |  
     |  AddProposal(...)
     |      AddProposal(self: WasmEdge.Configure, arg0: WasmEdge_Proposal) -> None
     |  
     |  CompilerGetOptimizationLevel(...)
     |      CompilerGetOptimizationLevel(self: WasmEdge.Configure) -> WasmEdge_CompilerOptimizationLevel
     |  
     |  CompilerGetOutputFormat(...)
     |      CompilerGetOutputFormat(self: WasmEdge.Configure) -> WasmEdge_CompilerOutputFormat
     |  
     |  CompilerIsDumpIR(...)
     |      CompilerIsDumpIR(self: WasmEdge.Configure) -> bool
     |  
     |  CompilerIsGenericBinary(...)
     |      CompilerIsGenericBinary(self: WasmEdge.Configure) -> bool
     |  
     |  CompilerIsInterruptible(...)
     |      CompilerIsInterruptible(self: WasmEdge.Configure) -> bool
     |  
     |  CompilerSetDumpIR(...)
     |      CompilerSetDumpIR(self: WasmEdge.Configure, arg0: bool) -> None
     |  
     |  CompilerSetGenericBinary(...)
     |      CompilerSetGenericBinary(self: WasmEdge.Configure, arg0: bool) -> None
     |  
     |  CompilerSetInterruptible(...)
     |      CompilerSetInterruptible(self: WasmEdge.Configure, arg0: bool) -> None
     |  
     |  CompilerSetOptimizationLevel(...)
     |      CompilerSetOptimizationLevel(self: WasmEdge.Configure, arg0: WasmEdge_CompilerOptimizationLevel) -> None
     |  
     |  CompilerSetOutputFormat(...)
     |      CompilerSetOutputFormat(self: WasmEdge.Configure, arg0: WasmEdge_CompilerOutputFormat) -> None
     |  
     |  GetMaxMemoryPage(...)
     |      GetMaxMemoryPage(self: WasmEdge.Configure) -> int
     |  
     |  HasHostRegistration(...)
     |      HasHostRegistration(self: WasmEdge.Configure, arg0: WasmEdge_HostRegistration) -> bool
     |  
     |  HasProposal(...)
     |      HasProposal(self: WasmEdge.Configure, arg0: WasmEdge_Proposal) -> bool
     |  
     |  RemoveHostRegistration(...)
     |      RemoveHostRegistration(self: WasmEdge.Configure, arg0: WasmEdge_HostRegistration) -> None
     |  
     |  RemoveProposal(...)
     |      RemoveProposal(self: WasmEdge.Configure, arg0: WasmEdge_Proposal) -> None
     |  
     |  SetMaxMemoryPage(...)
     |      SetMaxMemoryPage(self: WasmEdge.Configure, arg0: int) -> None
     |  
     |  StatisticsIsCostMeasuring(...)
     |      StatisticsIsCostMeasuring(self: WasmEdge.Configure) -> bool
     |  
     |  StatisticsIsInstructionCounting(...)
     |      StatisticsIsInstructionCounting(self: WasmEdge.Configure) -> bool
     |  
     |  StatisticsIsTimeMeasuring(...)
     |      StatisticsIsTimeMeasuring(self: WasmEdge.Configure) -> bool
     |  
     |  StatisticsSetCostMeasuring(...)
     |      StatisticsSetCostMeasuring(self: WasmEdge.Configure, arg0: bool) -> None
     |  
     |  StatisticsSetInstructionCounting(...)
     |      StatisticsSetInstructionCounting(self: WasmEdge.Configure, arg0: bool) -> None
     |  
     |  StatisticsSetTimeMeasuring(...)
     |      StatisticsSetTimeMeasuring(self: WasmEdge.Configure, arg0: bool) -> None
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Configure) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Executor(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Executor
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  RegisterImport(...)
     |      RegisterImport(self: WasmEdge.Executor, arg0: WasmEdge.Store, arg1: pysdk::import_object) -> WasmEdge.Result
     |  
     |  RegisterModule(...)
     |      RegisterModule(self: WasmEdge.Executor, arg0: WasmEdge.Store, arg1: WasmEdge.ASTModule, arg2: str) -> WasmEdge.Result
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Executor, arg0: WasmEdge.Configure) -> None
     |  
     |  instantiate(...)
     |      instantiate(self: WasmEdge.Executor, arg0: WasmEdge.Store, arg1: WasmEdge.ASTModule) -> WasmEdge.Result
     |  
     |  invoke(...)
     |      invoke(self: WasmEdge.Executor, arg0: WasmEdge.Store, arg1: str, arg2: list) -> tuple
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class ExportType(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      ExportType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetExternalName(...)
     |      GetExternalName(self: WasmEdge.ExportType) -> str
     |  
     |  GetExternalType(...)
     |      GetExternalType(self: WasmEdge.ExportType) -> WasmEdge.ExternalType
     |  
     |  GetFunctionType(...)
     |      GetFunctionType(self: WasmEdge.ExportType, arg0: pysdk::ASTModuleCxt) -> pysdk::FunctionTypeContext
     |  
     |  GetGlobalType(...)
     |      GetGlobalType(self: WasmEdge.ExportType, arg0: pysdk::ASTModuleCxt) -> pysdk::GlobalTypeCxt
     |  
     |  GetMemoryType(...)
     |      GetMemoryType(self: WasmEdge.ExportType, arg0: pysdk::ASTModuleCxt) -> pysdk::MemoryTypeCxt
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.ExportType) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class ExternalType(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  Func
     |  
     |  Glob
     |  
     |  Mem
     |  
     |  Tab
     |  
     |  Method resolution order:
     |      ExternalType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.ExternalType) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.ExternalType, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.ExternalType) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.ExternalType, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  Func = <ExternalType.Func: 0>
     |  
     |  Glob = <ExternalType.Glob: 3>
     |  
     |  Mem = <ExternalType.Mem: 2>
     |  
     |  Tab = <ExternalType.Tab: 1>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Function(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Function
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetType(...)
     |      GetType(self: WasmEdge.Function) -> WasmEdge.FunctionType
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Function, arg0: WasmEdge.FunctionType, arg1: function, arg2: int) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class FunctionType(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      FunctionType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetParamLen(...)
     |      GetParamLen(self: WasmEdge.FunctionType) -> int
     |  
     |  GetParamTypes(...)
     |      GetParamTypes(self: WasmEdge.FunctionType, arg0: int) -> list
     |  
     |  GetRetLen(...)
     |      GetRetLen(self: WasmEdge.FunctionType) -> int
     |  
     |  GetRetTypes(...)
     |      GetRetTypes(self: WasmEdge.FunctionType, arg0: int) -> list
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.FunctionType, arg0: list, arg1: list) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Global(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Global
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetGlobalType(...)
     |      GetGlobalType(self: WasmEdge.Global) -> WasmEdge.GlobalType
     |  
     |  GetValue(...)
     |      GetValue(self: WasmEdge.Global) -> WasmEdge.Value
     |  
     |  SetValue(...)
     |      SetValue(self: WasmEdge.Global, arg0: WasmEdge.Value) -> None
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Global) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class GlobalType(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      GlobalType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetMutability(...)
     |      GetMutability(self: WasmEdge.GlobalType) -> WasmEdge.Mutability
     |  
     |  GetValType(...)
     |      GetValType(self: WasmEdge.GlobalType) -> WasmEdge.Type
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.GlobalType, arg0: WasmEdge.Type, arg1: WasmEdge.Mutability) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Host(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  Wasi
     |  
     |  WasmEdge
     |  
     |  Method resolution order:
     |      Host
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.Host) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Host, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.Host) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.Host, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  Wasi = <Host.Wasi: 0>
     |  
     |  WasmEdge = <Host.WasmEdge: 1>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class ImportObject(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      ImportObject
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  AddFunction(...)
     |      AddFunction(self: WasmEdge.ImportObject, arg0: str, arg1: WasmEdge.Function) -> None
     |  
     |  AddGlobal(...)
     |      AddGlobal(self: WasmEdge.ImportObject, arg0: str, arg1: pysdk::Global) -> None
     |  
     |  AddMemory(...)
     |      AddMemory(self: WasmEdge.ImportObject, arg0: str, arg1: pysdk::Memory) -> None
     |  
     |  AddTable(...)
     |      AddTable(self: WasmEdge.ImportObject, arg0: str, arg1: pysdk::Table) -> None
     |  
     |  InitWASI(...)
     |      InitWASI(self: WasmEdge.ImportObject, arg0: tuple, arg1: tuple, arg2: tuple) -> None
     |  
     |  InitWasmEdgeProcess(...)
     |      InitWasmEdgeProcess(self: WasmEdge.ImportObject, arg0: tuple, arg1: bool) -> None
     |  
     |  WASIGetExitCode(...)
     |      WASIGetExitCode(self: WasmEdge.ImportObject) -> int
     |  
     |  __init__(...)
     |      __init__(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. __init__(self: WasmEdge.ImportObject, arg0: str) -> None
     |      
     |      2. __init__(self: WasmEdge.ImportObject, arg0: tuple, arg1: tuple, arg2: tuple) -> None
     |      
     |      3. __init__(self: WasmEdge.ImportObject, arg0: tuple, arg1: bool) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class ImportType(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      ImportType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetExternalName(...)
     |      GetExternalName(self: WasmEdge.ImportType) -> str
     |  
     |  GetExternalType(...)
     |      GetExternalType(self: WasmEdge.ImportType) -> WasmEdge.ExternalType
     |  
     |  GetFunctionType(...)
     |      GetFunctionType(self: WasmEdge.ImportType, arg0: WasmEdge.ASTModule) -> WasmEdge.FunctionType
     |  
     |  GetGlobalType(...)
     |      GetGlobalType(self: WasmEdge.ImportType, arg0: WasmEdge.ASTModule) -> pysdk::GlobalTypeCxt
     |  
     |  GetMemoryType(...)
     |      GetMemoryType(self: WasmEdge.ImportType, arg0: WasmEdge.ASTModule) -> pysdk::MemoryTypeCxt
     |  
     |  GetModuleName(...)
     |      GetModuleName(self: WasmEdge.ImportType) -> str
     |  
     |  GetTableType(...)
     |      GetTableType(self: WasmEdge.ImportType, arg0: WasmEdge.ASTModule) -> pysdk::TableTypeCxt
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.ImportType) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Limit(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Limit
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Limit, arg0: bool, arg1: int, arg2: int) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  HasMax
     |  
     |  Max
     |  
     |  Min
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Loader(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Loader
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Loader, arg0: WasmEdge.Configure) -> None
     |  
     |  parse(...)
     |      parse(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. parse(self: WasmEdge.Loader, arg0: WasmEdge.ASTModule, arg1: str) -> WasmEdge.Result
     |      
     |      2. parse(self: WasmEdge.Loader, arg0: WasmEdge.ASTModule, arg1: tuple) -> WasmEdge.Result
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Logging(pybind11_builtins.pybind11_object)
     |  Set the log level.
     |  
     |  Method resolution order:
     |      Logging
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Logging) -> None
     |  
     |  __str__(...)
     |      __str__(self: WasmEdge.Logging) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Static methods defined here:
     |  
     |  debug(...) from builtins.PyCapsule
     |      debug() -> None
     |      
     |      
     |      Set the log level to debug i.e. debug info is printed.
     |  
     |  error(...) from builtins.PyCapsule
     |      error() -> None
     |      
     |      
     |      Set the log level to error i.e. no debug info is printed.
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Memory(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Memory
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetData(...)
     |      GetData(self: WasmEdge.Memory, arg0: int, arg1: int) -> tuple
     |  
     |  GetMemoryType(...)
     |      GetMemoryType(self: WasmEdge.Memory) -> WasmEdge.MemoryType
     |  
     |  GetPageSize(...)
     |      GetPageSize(self: WasmEdge.Memory) -> int
     |  
     |  GrowPage(...)
     |      GrowPage(self: WasmEdge.Memory, arg0: int) -> WasmEdge.Result
     |  
     |  SetData(...)
     |      SetData(self: WasmEdge.Memory, arg0: tuple, arg1: int) -> WasmEdge.Result
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Memory, arg0: WasmEdge.MemoryType) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class MemoryType(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      MemoryType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetLimit(...)
     |      GetLimit(self: WasmEdge.MemoryType) -> WasmEdge.Limit
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.MemoryType, arg0: WasmEdge.Limit) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Mutability(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  Var
     |  
     |  Const
     |  
     |  Method resolution order:
     |      Mutability
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.Mutability) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Mutability, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.Mutability) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.Mutability, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  Const = <Mutability.Const: 0>
     |  
     |  Var = <Mutability.Var: 1>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Optimization(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  O0
     |  
     |  O1
     |  
     |  O2
     |  
     |  O3
     |  
     |  Os
     |  
     |  Oz
     |  
     |  Method resolution order:
     |      Optimization
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.Optimization) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Optimization, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.Optimization) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.Optimization, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  O0 = <Optimization.O0: 0>
     |  
     |  O1 = <Optimization.O1: 1>
     |  
     |  O2 = <Optimization.O2: 2>
     |  
     |  O3 = <Optimization.O3: 3>
     |  
     |  Os = <Optimization.Os: 4>
     |  
     |  Oz = <Optimization.Oz: 5>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Proposal(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  ImportExportMutGlobals
     |  
     |  NonTrapFloatToIntConversions
     |  
     |  BulkMemoryOperations
     |  
     |  ReferenceTypes
     |  
     |  SIMD
     |  
     |  TailCall
     |  
     |  Annotations
     |  
     |  Memory64
     |  
     |  Threads
     |  
     |  ExceptionHandling
     |  
     |  FunctionReferences
     |  
     |  Method resolution order:
     |      Proposal
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.Proposal) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Proposal, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.Proposal) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.Proposal, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  Annotations = <Proposal.Annotations: 9>
     |  
     |  BulkMemoryOperations = <Proposal.BulkMemoryOperations: 4>
     |  
     |  ExceptionHandling = <Proposal.ExceptionHandling: 11>
     |  
     |  FunctionReferences = <Proposal.FunctionReferences: 13>
     |  
     |  ImportExportMutGlobals = <Proposal.ImportExportMutGlobals: 0>
     |  
     |  Memory64 = <Proposal.Memory64: 10>
     |  
     |  NonTrapFloatToIntConversions = <Proposal.NonTrapFloatToIntConversions:...
     |  
     |  ReferenceTypes = <Proposal.ReferenceTypes: 5>
     |  
     |  SIMD = <Proposal.SIMD: 6>
     |  
     |  TailCall = <Proposal.TailCall: 7>
     |  
     |  Threads = <Proposal.Threads: 12>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class RefType(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  FuncRef
     |  
     |  ExternRef
     |  
     |  Method resolution order:
     |      RefType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.RefType) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.RefType, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.RefType) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.RefType, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  ExternRef = <RefType.ExternRef: 111>
     |  
     |  FuncRef = <RefType.FuncRef: 112>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Result(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Result
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __bool__(...)
     |      __bool__(self: WasmEdge.Result) -> bool
     |  
     |  __init__(...)
     |      __init__(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. __init__(self: WasmEdge.Result) -> None
     |      
     |      2. __init__(self: WasmEdge.Result, arg0: int) -> None
     |  
     |  __str__(...)
     |      __str__(self: WasmEdge.Result) -> str
     |  
     |  code(...)
     |      code(self: WasmEdge.Result) -> int
     |  
     |  message(...)
     |      message(self: WasmEdge.Result) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Statistics(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Statistics
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetInstrCount(...)
     |      GetInstrCount(self: WasmEdge.Statistics) -> int
     |  
     |  GetInstrPerSecond(...)
     |      GetInstrPerSecond(self: WasmEdge.Statistics) -> float
     |  
     |  GetTotalCost(...)
     |      GetTotalCost(self: WasmEdge.Statistics) -> int
     |  
     |  SetCostLimit(...)
     |      SetCostLimit(self: WasmEdge.Statistics, arg0: int) -> None
     |  
     |  SetCostTable(...)
     |      SetCostTable(self: WasmEdge.Statistics, arg0: tuple) -> None
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Statistics) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Store(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Store
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  FindFunction(...)
     |      FindFunction(self: WasmEdge.Store, arg0: str) -> pysdk::Function
     |  
     |  FindFunctionRegistered(...)
     |      FindFunctionRegistered(self: WasmEdge.Store, arg0: str, arg1: str) -> pysdk::Function
     |  
     |  FindGlobal(...)
     |      FindGlobal(self: WasmEdge.Store, arg0: str) -> pysdk::Global
     |  
     |  FindGlobalRegistered(...)
     |      FindGlobalRegistered(self: WasmEdge.Store, arg0: str, arg1: str) -> pysdk::Global
     |  
     |  FindMemory(...)
     |      FindMemory(self: WasmEdge.Store, arg0: str) -> pysdk::Memory
     |  
     |  FindMemoryRegistered(...)
     |      FindMemoryRegistered(self: WasmEdge.Store, arg0: str, arg1: str) -> pysdk::Memory
     |  
     |  FindTable(...)
     |      FindTable(self: WasmEdge.Store, arg0: str) -> pysdk::Table
     |  
     |  FindTableRegistered(...)
     |      FindTableRegistered(self: WasmEdge.Store, arg0: str, arg1: str) -> pysdk::Table
     |  
     |  ListFunction(...)
     |      ListFunction(self: WasmEdge.Store, arg0: int) -> list
     |  
     |  ListFunctionLength(...)
     |      ListFunctionLength(self: WasmEdge.Store) -> int
     |  
     |  ListFunctionRegistered(...)
     |      ListFunctionRegistered(self: WasmEdge.Store, arg0: str, arg1: int) -> list
     |  
     |  ListFunctionRegisteredLength(...)
     |      ListFunctionRegisteredLength(self: WasmEdge.Store, arg0: str) -> int
     |  
     |  ListGlobal(...)
     |      ListGlobal(self: WasmEdge.Store, arg0: int) -> list
     |  
     |  ListGlobalLength(...)
     |      ListGlobalLength(self: WasmEdge.Store) -> int
     |  
     |  ListGlobalRegistered(...)
     |      ListGlobalRegistered(self: WasmEdge.Store, arg0: str, arg1: int) -> list
     |  
     |  ListGlobalRegisteredLength(...)
     |      ListGlobalRegisteredLength(self: WasmEdge.Store, arg0: str) -> int
     |  
     |  ListMemory(...)
     |      ListMemory(self: WasmEdge.Store, arg0: int) -> list
     |  
     |  ListMemoryLength(...)
     |      ListMemoryLength(self: WasmEdge.Store) -> int
     |  
     |  ListMemoryRegistered(...)
     |      ListMemoryRegistered(self: WasmEdge.Store, arg0: str, arg1: int) -> list
     |  
     |  ListMemoryRegisteredLength(...)
     |      ListMemoryRegisteredLength(self: WasmEdge.Store, arg0: str) -> int
     |  
     |  ListModule(...)
     |      ListModule(self: WasmEdge.Store, arg0: int) -> list
     |  
     |  ListModuleLength(...)
     |      ListModuleLength(self: WasmEdge.Store) -> int
     |  
     |  ListTable(...)
     |      ListTable(self: WasmEdge.Store, arg0: int) -> list
     |  
     |  ListTableLength(...)
     |      ListTableLength(self: WasmEdge.Store) -> int
     |  
     |  ListTableRegistered(...)
     |      ListTableRegistered(self: WasmEdge.Store, arg0: str, arg1: int) -> list
     |  
     |  ListTableRegisteredLength(...)
     |      ListTableRegisteredLength(self: WasmEdge.Store, arg0: str) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Store) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Table(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Table
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetData(...)
     |      GetData(self: WasmEdge.Table, arg0: int) -> tuple
     |  
     |  GetSize(...)
     |      GetSize(self: WasmEdge.Table) -> int
     |  
     |  GetType(...)
     |      GetType(self: WasmEdge.Table) -> WasmEdge.TableType
     |  
     |  GrowSize(...)
     |      GrowSize(self: WasmEdge.Table, arg0: int) -> WasmEdge.Result
     |  
     |  SetData(...)
     |      SetData(self: WasmEdge.Table, arg0: WasmEdge.Value, arg1: int) -> WasmEdge.Result
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Table, arg0: WasmEdge.TableType) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class TableType(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      TableType
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  GetLimit(...)
     |      GetLimit(self: WasmEdge.TableType) -> WasmEdge.Limit
     |  
     |  GetRefType(...)
     |      GetRefType(self: WasmEdge.TableType) -> WasmEdge.RefType
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.TableType, arg0: WasmEdge.RefType, arg1: WasmEdge.Limit) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Type(pybind11_builtins.pybind11_object)
     |  Members:
     |  
     |  I32
     |  
     |  I64
     |  
     |  F32
     |  
     |  F64
     |  
     |  V128
     |  
     |  FuncRef
     |  
     |  ExternRef
     |  
     |  Method resolution order:
     |      Type
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __eq__(...)
     |      __eq__(self: object, other: object) -> bool
     |  
     |  __getstate__(...)
     |      __getstate__(self: object) -> int
     |  
     |  __hash__(...)
     |      __hash__(self: object) -> int
     |  
     |  __index__(...)
     |      __index__(self: WasmEdge.Type) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Type, value: int) -> None
     |  
     |  __int__(...)
     |      __int__(self: WasmEdge.Type) -> int
     |  
     |  __ne__(...)
     |      __ne__(self: object, other: object) -> bool
     |  
     |  __repr__(...)
     |      __repr__(self: object) -> str
     |  
     |  __setstate__(...)
     |      __setstate__(self: WasmEdge.Type, state: int) -> None
     |  
     |  __str__ = name(...)
     |      name(self: handle) -> str
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  __members__
     |  
     |  name
     |      name(self: handle) -> str
     |  
     |  value
     |  
     |  ----------------------------------------------------------------------
     |  Data and other attributes defined here:
     |  
     |  ExternRef = <Type.ExternRef: 111>
     |  
     |  F32 = <Type.F32: 125>
     |  
     |  F64 = <Type.F64: 124>
     |  
     |  FuncRef = <Type.FuncRef: 112>
     |  
     |  I32 = <Type.I32: 127>
     |  
     |  I64 = <Type.I64: 126>
     |  
     |  V128 = <Type.V128: 123>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class VM(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      VM
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  AsyncExecute(...)
     |      AsyncExecute(self: WasmEdge.VM, arg0: str, arg1: tuple) -> pysdk::Async
     |  
     |  AsyncExecuteRegistered(...)
     |      AsyncExecuteRegistered(self: WasmEdge.VM, arg0: str, arg1: str, arg2: tuple) -> pysdk::Async
     |  
     |  AsyncRunWasmFromASTModule(...)
     |      AsyncRunWasmFromASTModule(self: WasmEdge.VM, arg0: WasmEdge.ASTModule, arg1: str, arg2: tuple) -> pysdk::Async
     |  
     |  AsyncRunWasmFromBuffer(...)
     |      AsyncRunWasmFromBuffer(self: WasmEdge.VM, arg0: tuple, arg1: tuple, arg2: str) -> pysdk::Async
     |  
     |  AsyncRunWasmFromFile(...)
     |      AsyncRunWasmFromFile(self: WasmEdge.VM, arg0: str, arg1: str, arg2: tuple) -> pysdk::Async
     |  
     |  Execute(...)
     |      Execute(self: WasmEdge.VM, arg0: str, arg1: tuple, arg2: int) -> tuple
     |  
     |  ExecuteRegistered(...)
     |      ExecuteRegistered(self: WasmEdge.VM, arg0: str, arg1: str, arg2: list, arg3: int) -> tuple
     |  
     |  GetFunctionList(...)
     |      GetFunctionList(self: WasmEdge.VM, arg0: int) -> dict
     |  
     |  GetFunctionListLength(...)
     |      GetFunctionListLength(self: WasmEdge.VM) -> int
     |  
     |  GetFunctionType(...)
     |      GetFunctionType(self: WasmEdge.VM, arg0: str) -> pysdk::FunctionTypeContext
     |  
     |  GetFunctionTypeRegistered(...)
     |      GetFunctionTypeRegistered(self: WasmEdge.VM, arg0: str, arg1: str) -> pysdk::FunctionTypeContext
     |  
     |  GetImportModuleContext(...)
     |      GetImportModuleContext(self: WasmEdge.VM, arg0: WasmEdge.Host) -> pysdk::import_object
     |  
     |  GetStatistics(...)
     |      GetStatistics(self: WasmEdge.VM) -> WasmEdge.Statistics
     |  
     |  GetStoreContext(...)
     |      GetStoreContext(self: WasmEdge.VM) -> WasmEdge.Store
     |  
     |  Instantiate(...)
     |      Instantiate(self: WasmEdge.VM) -> WasmEdge.Result
     |  
     |  LoadWasmFromASTModule(...)
     |      LoadWasmFromASTModule(self: WasmEdge.VM, arg0: WasmEdge.ASTModule) -> WasmEdge.Result
     |  
     |  LoadWasmFromBuffer(...)
     |      LoadWasmFromBuffer(self: WasmEdge.VM, arg0: tuple) -> WasmEdge.Result
     |  
     |  LoadWasmFromFile(...)
     |      LoadWasmFromFile(self: WasmEdge.VM, arg0: str) -> WasmEdge.Result
     |  
     |  RegisterModuleFromASTModule(...)
     |      RegisterModuleFromASTModule(self: WasmEdge.VM, arg0: str, arg1: WasmEdge.ASTModule) -> WasmEdge.Result
     |  
     |  RegisterModuleFromBuffer(...)
     |      RegisterModuleFromBuffer(self: WasmEdge.VM, arg0: str, arg1: tuple) -> WasmEdge.Result
     |  
     |  RegisterModuleFromFile(...)
     |      RegisterModuleFromFile(self: WasmEdge.VM, arg0: str, arg1: str) -> WasmEdge.Result
     |  
     |  RegisterModuleFromImport(...)
     |      RegisterModuleFromImport(self: WasmEdge.VM, arg0: pysdk::import_object) -> WasmEdge.Result
     |  
     |  RunWasmFromASTModule(...)
     |      RunWasmFromASTModule(self: WasmEdge.VM, arg0: WasmEdge.ASTModule, arg1: str, arg2: tuple, arg3: int) -> tuple
     |  
     |  RunWasmFromBuffer(...)
     |      RunWasmFromBuffer(self: WasmEdge.VM, arg0: tuple, arg1: tuple, arg2: str, arg3: int) -> tuple
     |  
     |  RunWasmFromFile(...)
     |      RunWasmFromFile(self: WasmEdge.VM, arg0: str, arg1: str, arg2: tuple, arg3: int) -> tuple
     |  
     |  Validate(...)
     |      Validate(self: WasmEdge.VM) -> WasmEdge.Result
     |  
     |  __init__(...)
     |      __init__(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. __init__(self: WasmEdge.VM) -> None
     |      
     |      2. __init__(self: WasmEdge.VM, arg0: WasmEdge.Configure) -> None
     |      
     |      3. __init__(self: WasmEdge.VM, arg0: WasmEdge.Store) -> None
     |      
     |      4. __init__(self: WasmEdge.VM, arg0: WasmEdge.Configure, arg1: WasmEdge.Store) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Validator(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Validator
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Validator, arg0: WasmEdge.Configure) -> None
     |  
     |  validate(...)
     |      validate(self: WasmEdge.Validator, arg0: WasmEdge.ASTModule) -> WasmEdge.Result
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Value(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Value
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Value, arg0: object, arg1: WasmEdge.Type) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  Type
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors defined here:
     |  
     |  Value
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.

FUNCTIONS
    AsyncDelete(...) method of builtins.PyCapsule instance
        AsyncDelete(arg0: WasmEdge.Async) -> None
    
    VersionGet(...) method of builtins.PyCapsule instance
        VersionGet() -> str
        
        
        Get the version string of the WasmEdge C API.
        
        :return: String of version
        :rtype: str
    
    VersionGetMajor(...) method of builtins.PyCapsule instance
        VersionGetMajor() -> int
        
        
        Get the major version value of the WasmEdge C API.
        
        :return: Int of major version
        :rtype: int
    
    VersionGetMinor(...) method of builtins.PyCapsule instance
        VersionGetMinor() -> int
        
        
        Get the minor version value of the WasmEdge C API.
        
        :return: Int of minor version
        :rtype: int
    
    VersionGetPatch(...) method of builtins.PyCapsule instance
        VersionGetPatch() -> int
        
        
        Get the patch version value of the WasmEdge C API.
        
        :return: Int of patch version
        :rtype: int

DATA
    Annotations = <Proposal.Annotations: 9>
    BulkMemoryOperations = <Proposal.BulkMemoryOperations: 4>
    Const = <Mutability.Const: 0>
    ExceptionHandling = <Proposal.ExceptionHandling: 11>
    ExternRef = <RefType.ExternRef: 111>
    F32 = <Type.F32: 125>
    F64 = <Type.F64: 124>
    Func = <ExternalType.Func: 0>
    FuncRef = <RefType.FuncRef: 112>
    FunctionReferences = <Proposal.FunctionReferences: 13>
    Glob = <ExternalType.Glob: 3>
    I32 = <Type.I32: 127>
    I64 = <Type.I64: 126>
    ImportExportMutGlobals = <Proposal.ImportExportMutGlobals: 0>
    Mem = <ExternalType.Mem: 2>
    Memory64 = <Proposal.Memory64: 10>
    Native = <CompilerOutput.Native: 0>
    NonTrapFloatToIntConversions = <Proposal.NonTrapFloatToIntConversions:...
    O0 = <Optimization.O0: 0>
    O1 = <Optimization.O1: 1>
    O2 = <Optimization.O2: 2>
    O3 = <Optimization.O3: 3>
    Os = <Optimization.Os: 4>
    Oz = <Optimization.Oz: 5>
    ReferenceTypes = <Proposal.ReferenceTypes: 5>
    SIMD = <Proposal.SIMD: 6>
    Tab = <ExternalType.Tab: 1>
    TailCall = <Proposal.TailCall: 7>
    Threads = <Proposal.Threads: 12>
    V128 = <Type.V128: 123>
    Var = <Mutability.Var: 1>
    Wasi = <Host.Wasi: 0>
    Wasm = <CompilerOutput.Wasm: 1>
    WasmEdge = <Host.WasmEdge: 1>

FILE
    /home/satacker/wasm_work/WasmEdge/bindings/python/WasmEdge.cpython-38-x86_64-linux-gnu.so


```
