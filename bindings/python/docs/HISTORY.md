(unreleased)
------------
- [PySDK] makefile: build parallel and build. [Shreyas Atre]
- [PySDK] Stub files improvement. [Shreyas Atre]
- [PySDK] Completion of Async. [Shreyas Atre]
- [PySDK] Added Compiler Class wrapper. [Shreyas Atre]
- [PySDK] Complete executor. [Shreyas Atre]
- [PySDK] Import Type Complete. [Shreyas Atre]
- [PySDK] Complete Global Type Context. [Shreyas Atre]
- [PySDK] Table Type complete. [Shreyas Atre]
- [PySDK] Complete ImportObject. [Shreyas Atre]
- [PySDK] Complete Global Instance. [Shreyas Atre]
- [PySDK] Memory Instance Complete. [Shreyas Atre]
- [PySDK] Completed Export, AST Module. [Shreyas Atre]
- [PySDK] Complete Store Context API. [Shreyas Atre]
- [PySDK] Sphinx Docs RTD theme. [Shreyas Atre]
- [PySDK] Complete Statistics Context wrapper. [Shreyas Atre]
- [PySDK] Completed configure. [Shreyas Atre]
- [PySDK] Restructure doc_strings. [Shreyas Atre]
- [PySDK] Consistent with C-API. [Shreyas Atre]
- [PySDK][Docs] Use Sphinx RTD for API Documentation. [Shreyas Atre]
- [PySDK] Line number, file name, in runtime error. [Shreyas Atre]
- [PySDK] Made it consistent with WasmEdge-C API. [Shreyas Atre]

  * Necessary stuff to implement WasmEdge-bindgen
- [PySDK] Renamed ImportObject.add to ImportObject.AddFunction. [Shreyas
  Atre]
- [PySDK] Added VM.Execute, VM.Instantiate, VM.Validate. [Shreyas Atre]
- [PySDK] PyPi Release 0.2.1. [Shreyas Atre]
- [PySDK] Added ExternalType enum and ImportType class. [Shreyas Atre]

  * ExternalType Enum has name overlapping which creates import errors
  ```
    import WasmEdge
  E   ImportError: generic_type: cannot initialize type "Function": an object with that name is already defined
  ```
- [PySDK] Made VM api consistent with WasmEdge C API. [Shreyas Atre]
- [PySDK] Made VM.RunWasmFromBuffer name and signature consistent with C
  SDK. [Shreyas Atre]
- [PySDK] Added VM.RunWasmFromASTModule. [Shreyas Atre]
- [PySDK] Renamed functions for consistency. [Shreyas Atre]
- [PySDK] Added LoadWasmFromFile. [Shreyas Atre]
- [PySDK] Added VM.LoadWasmFromBuffer. [Shreyas Atre]
- [PySDK] Added Statistics Context and improved VM. [Shreyas Atre]
- [PySDK] Added ExecuteRegistered method to VM. [Shreyas Atre]

  * Removed max-fail in test cases
- [PySDK] Added VM::ExecuteRegistered and fixed host function execution.
  [Shreyas Atre]
- [PySDK] Added VM.ExecuteRegistered. [Shreyas Atre]
- [PySDK] Fundamental Improvements to SDK. [Shreyas Atre]

  * Made pywasmedge/src/function.cpp significant change:
  	Function that is written in python should accept
  `WasmEdge.Value(s)` and return a tuple of `WasmEdge.Result(0)`
  `and WasmEdge.Value(s).`

  * Incorporated changes to function in pywasmedge/src/import_object.cpp

  * Added arguments to `WasmEdge.Result`

  * Added additional definitions to `WasmEdge.Value` class

  * Changed `WasmEdge.VM` as per new `WasmEdge.Value`

  * Added test for function instance and host function
- [PySDK] Added FunctionTypeContext. [Shreyas Atre]
- Release: version 0.2.0 🚀 [Shreyas Atre]
- Release: version 0.2.0 🚀 [Shreyas Atre]
- [PySDK] Added Table Context and Instance. [Shreyas Atre]

  * Also improved Value Class
- [PySDK] Added memory instance support. [Shreyas Atre]
- [PySDK] Added Mutability, GlobalType and MemoryType Context Support.
  [Shreyas Atre]
- [PySDK] Added WasmEdge_Limit support. [Shreyas Atre]
- [PySDK] Improve WasmEdge_Value. [Shreyas Atre]
- [PySDK] Runtime Exeception incase of parameter length mismatch.
  [Shreyas Atre]
- [PySDK] Added support for funcref. [Shreyas Atre]
- [PySDK] Added Reference Class to support externref. [Shreyas Atre]
- [PySDK] Added RefType Enum Class. [Shreyas Atre]
- [PySDK] CMake CXX STD to 11. [Shreyas Atre]

  * Not using any features of 17
- [PySDK] PyPi Testing published. [Shreyas Atre]
- [PySDK] VM API made consistent. [Shreyas Atre]
- [PySDK] Exclude pybind11 from getting formatted. [Shreyas Atre]
- [PySDK] Added Executor support. [Shreyas Atre]

  * Fixed ASTModuleCxt address of pointer function
  * Used the above function in Loader
- [PySDK] Added Validator support. [Shreyas Atre]
- [PySDK] Added Loader support. [Shreyas Atre]
- [PySDK] Added AST Module Context support. [Shreyas Atre]
- [PySDK] Cleanup: Code Cosmetic changes. [Shreyas Atre]

  * Cleanup and re organizing into multiple files.
- [PySDK] Added WasmEdge_Value Support and Corresponding tests. [Shreyas
  Atre]
- [PySDK] Fix: Host function execution. [Shreyas Atre]

  * Flow of invocation from https://github.com/WasmEdge/WasmEdge/pull/633#issuecomment-986660764
- [PySDK] List registered functions functions. [Shreyas Atre]
- [PySDK] Fix: Finding functions in registered modules. [Shreyas Atre]
- [PySDK] Fix: function overloading issue. [Shreyas Atre]

  * Temporary workaround
- [PySDK] Fix For Host Function. [Shreyas Atre]

  * Host Function cannot be passed if it is a member function.
- [PySDK] Module and Function Support. [Shreyas Atre]

  * Incomplete Implementation
  * Build Error surrounds arround the host function signature.
- [PySDK] Added compiler optimizations and output format. [Shreyas Atre]
- [PySDK] Use PyBind11 instead of Boost.Python. [Shreyas Atre]

  * Suggestions from https://github.com/WasmEdge/WasmEdge/issues/650#issue-1052718123
  * Boost.Python has lesser functionality than PyBind11
  * Reduced compile time
- [PySDK] Specify Types of parameters and return values in advance.
  [Shreyas Atre]

  * Utilize the `Type` class implemented in python and specify as arguments in run function.
  * https://github.com/WasmEdge/WasmEdge/issues/645
- [PySDK] Fix:
  https://github.com/WasmEdge/WasmEdge/issues/644#issue-1050904719.
  [Shreyas Atre]

  * Added missing host registration for wasi in test
  * Added Value in python
- [PySDK] Overload Python vm.run() function without return length.
  [Shreyas Atre]

  * Use Step by step execution of WasmEdge VM and WasmEdge Function API calls to get the same. Return a boost::python::tuple of result and return list.
  * Suggestions from https://github.com/WasmEdge/WasmEdge/issues/642

  Status:
  * Tests fail https://github.com/WasmEdge/WasmEdge/issues/644
- [PySDK] Remove redundant definitions of enums. [Shreyas Atre]

  * Suggestions from https://github.com/WasmEdge/WasmEdge/pull/633#issuecomment-964505682
- [PySDK] Changed __str__ for logging. [Shreyas Atre]

  * __str__ is not to be confused with __doc__
  * Minor changes in Makefile
- [PySDK] Reference Issue fix. [Shreyas Atre]

  * Previously the Configure and Store passed to VM would make the program crash.
- [PySDK] Docs: Added pydoc and latest example. [Shreyas Atre]

  * Autogenerated pydoc output
  * Latest run example
- [PySDK] Several Changes. [Shreyas Atre]

  * Moved str definitions to separate doc_strings header
  * Created major Wrappers around Store,Configure and VM contexts
  * There is a minor issue in passing Store and Configure objects to VM
  * Passes basic Fibonacci test
- [PySDK] Debug flag addition. [Shreyas Atre]
- [PySDK] Configure class: Add host registration. [Shreyas Atre]

  * Overloading of python class Configure to support adding and removing of Host.
  * Wraps Host Registration.
- [PySDK] Configure, Proposal support. [Shreyas Atre]

  * Adds configuration support which is an api wrapped over contexts.
  * Proposal are just enums
- [PySDK] Boilerplate for PySDK based on Boost. [Shreyas Atre]

  * Currently it overloads WasmEdge Result as a boolean and supports version
  as a method.
  * Logging is implemented as a class.
  * Result supports message and code methods.
- [PySDK] Docs: Fix mkdocs yaml for HISTORY. [Shreyas Atre]
- [PySDK] Docs: Fix HISTORY generation. [Shreyas Atre]
- [PySDK] Version String function. [Shreyas Atre]

  * Returns version string
- [Docs] Fix docs example. [Shreyas Atre]
- [Test] PySDK: Test commit. [Shreyas Atre]

  * It is a very basic template for python port
- Initial Commit for python SDK. [Shreyas Atre]


