```
Help on module WasmEdge:

NAME
    WasmEdge

CLASSES
    pybind11_builtins.pybind11_object(builtins.object)
        ASTModule
        CompilerOutput
        Configure
        Executor
        Function
        GlobalType
        Host
        ImportObject
        Limit
        Loader
        Logging
        Memory
        MemoryType
        Mutability
        Optimization
        Proposal
        Ref
        RefType
        Result
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
     |  __init__(...)
     |      __init__(self: WasmEdge.ASTModule) -> None
     |  
     |  exports(...)
     |      exports(self: WasmEdge.ASTModule) -> list
     |  
     |  imports(...)
     |      imports(self: WasmEdge.ASTModule) -> list
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
     |  Method resolution order:
     |      Configure
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Configure) -> None
     |  
     |  add(...)
     |      add(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. add(self: WasmEdge.Configure, arg0: WasmEdge_Proposal) -> None
     |      
     |      2. add(self: WasmEdge.Configure, arg0: WasmEdge_HostRegistration) -> None
     |  
     |  remove(...)
     |      remove(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. remove(self: WasmEdge.Configure, arg0: WasmEdge_Proposal) -> None
     |      
     |      2. remove(self: WasmEdge.Configure, arg0: WasmEdge_HostRegistration) -> None
     |  
     |  ----------------------------------------------------------------------
     |  Data descriptors defined here:
     |  
     |  max_paging
     |  
     |  optimization_level
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
    
    class Function(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Function
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Function, arg0: function) -> None
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
     |  __init__(...)
     |      __init__(self: WasmEdge.ImportObject, arg0: str) -> None
     |  
     |  add(...)
     |      add(self: WasmEdge.ImportObject, arg0: WasmEdge.Function, arg1: str) -> None
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
     |  error(...) from builtins.PyCapsule
     |      error() -> None
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
     |  Annotations = <Proposal.Annotations: 8>
     |  
     |  BulkMemoryOperations = <Proposal.BulkMemoryOperations: 4>
     |  
     |  ExceptionHandling = <Proposal.ExceptionHandling: 11>
     |  
     |  FunctionReferences = <Proposal.FunctionReferences: 12>
     |  
     |  ImportExportMutGlobals = <Proposal.ImportExportMutGlobals: 0>
     |  
     |  Memory64 = <Proposal.Memory64: 9>
     |  
     |  NonTrapFloatToIntConversions = <Proposal.NonTrapFloatToIntConversions:...
     |  
     |  ReferenceTypes = <Proposal.ReferenceTypes: 5>
     |  
     |  SIMD = <Proposal.SIMD: 6>
     |  
     |  TailCall = <Proposal.TailCall: 7>
     |  
     |  Threads = <Proposal.Threads: 10>
     |  
     |  ----------------------------------------------------------------------
     |  Static methods inherited from pybind11_builtins.pybind11_object:
     |  
     |  __new__(*args, **kwargs) from pybind11_builtins.pybind11_type
     |      Create and return a new object.  See help(type) for accurate signature.
    
    class Ref(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Ref
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  FuncIdx(...)
     |      FuncIdx(self: WasmEdge.Ref) -> int
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Ref, type: object, ref_obj: object = None) -> None
     |  
     |  isNull(...)
     |      isNull(self: WasmEdge.Ref) -> bool
     |  
     |  ----------------------------------------------------------------------
     |  Readonly properties defined here:
     |  
     |  Type
     |  
     |  Value
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
     |      __init__(self: WasmEdge.Result) -> None
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
    
    class Store(pybind11_builtins.pybind11_object)
     |  Method resolution order:
     |      Store
     |      pybind11_builtins.pybind11_object
     |      builtins.object
     |  
     |  Methods defined here:
     |  
     |  __init__(...)
     |      __init__(self: WasmEdge.Store) -> None
     |  
     |  listFunctions(...)
     |      listFunctions(self: WasmEdge.Store) -> list
     |  
     |  listModules(...)
     |      listModules(self: WasmEdge.Store) -> list
     |  
     |  listRegisteredFunctions(...)
     |      listRegisteredFunctions(self: WasmEdge.Store, arg0: str) -> list
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
     |  ListExportedFunctions(...)
     |      ListExportedFunctions(self: WasmEdge.VM) -> list
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
     |  register(...)
     |      register(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. register(self: WasmEdge.VM, arg0: str, arg1: WasmEdge.ASTModule) -> WasmEdge.Result
     |      
     |      2. register(self: WasmEdge.VM, arg0: str, arg1: tuple) -> WasmEdge.Result
     |      
     |      3. register(self: WasmEdge.VM, arg0: str, arg1: str) -> WasmEdge.Result
     |      
     |      4. register(self: WasmEdge.VM, arg0: pysdk::import_object) -> WasmEdge.Result
     |  
     |  run(...)
     |      run(*args, **kwargs)
     |      Overloaded function.
     |      
     |      1. run(self: WasmEdge.VM, arg0: object, arg1: object, arg2: object, arg3: object, arg4: object) -> tuple
     |      
     |      2. run(self: WasmEdge.VM, arg0: object, arg1: object, arg2: object) -> tuple
     |      
     |      3. run(self: WasmEdge.VM, arg0: object, arg1: object, arg2: object, arg3: str) -> tuple
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
    version(...) method of builtins.PyCapsule instance
        version() -> str

DATA
    Annotations = <Proposal.Annotations: 8>
    BulkMemoryOperations = <Proposal.BulkMemoryOperations: 4>
    Const = <Mutability.Const: 0>
    ExceptionHandling = <Proposal.ExceptionHandling: 11>
    ExternRef = <RefType.ExternRef: 111>
    F32 = <Type.F32: 125>
    F64 = <Type.F64: 124>
    FuncRef = <RefType.FuncRef: 112>
    FunctionReferences = <Proposal.FunctionReferences: 12>
    I32 = <Type.I32: 127>
    I64 = <Type.I64: 126>
    ImportExportMutGlobals = <Proposal.ImportExportMutGlobals: 0>
    Memory64 = <Proposal.Memory64: 9>
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
    TailCall = <Proposal.TailCall: 7>
    Threads = <Proposal.Threads: 10>
    V128 = <Type.V128: 123>
    Var = <Mutability.Var: 1>
    Wasi = <Host.Wasi: 0>
    Wasm = <CompilerOutput.Wasm: 1>
    WasmEdge = <Host.WasmEdge: 1>

FILE
    /home/satacker/wasm_work/WasmEdge/bindings/python/WasmEdge.cpython-38-x86_64-linux-gnu.so


