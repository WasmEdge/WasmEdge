#ifndef WASMEDGE_DOC_STRINGS_HPP
#define WASMEDGE_DOC_STRINGS_HPP

namespace pysdk {

namespace doc {

constexpr const char *module = R"pbdoc(
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
           Store
           Statistics
           Optimization
           CompilerOutput
           Type
           ExternalType
           RefType
           Value
           Ref
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
    )pbdoc";

constexpr const char *VersionGet = R"pbdoc(
                Get the version string of the WasmEdge C API.

                :return: String of version
                :rtype: str
                )pbdoc";

constexpr const char *VersionGetMajor = R"pbdoc(
                Get the major version value of the WasmEdge C API.

                :return: Int of major version
                :rtype: int
                )pbdoc";

constexpr const char *VersionGetMinor = R"pbdoc(
                Get the minor version value of the WasmEdge C API.

                :return: Int of minor version
                :rtype: int
                )pbdoc";

constexpr const char *VersionGetPatch = R"pbdoc(
                Get the patch version value of the WasmEdge C API.

                :return: Int of patch version
                :rtype: int
                )pbdoc";

constexpr const char *Logging = R"pbdoc(
                Set the log level.
                )pbdoc";

constexpr const char *error = R"pbdoc(
                Set the log level to error i.e. no debug info is printed.
                )pbdoc";

constexpr const char *debug = R"pbdoc(
                Set the log level to debug i.e. debug info is printed.
                )pbdoc";

constexpr const char *Configure = R"pbdoc(
                The configuration context, `WasmEdge_ConfigureContext`, 
                is wrapped in the class `Configure` manages the 
                configurations for `Loader`, `Validator`, `Executor`,
                `VM`, and `Compiler`. Developers can adjust the settings
                about the proposals, VM host pre-registrations (such as 
                `WASI`), and AOT compiler options, and then apply the
                `Configure` context to create other runtime contexts.
                )pbdoc";
} // namespace doc
} // namespace pysdk

#endif // WASMEDGE_DOC_STRINGS_HPP