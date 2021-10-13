#ifndef WASMEDGE_DOC_STRINGS_HPP
#define WASMEDGE_DOC_STRINGS_HPP

namespace pysdk {

const char *vm_str =
    "                       |========================|\n"
    "              |------->|      VM: Initiated     |\n"
    "              |        |========================|\n"
    "              |                    |\n"
    "              |                 LoadWasm\n"
    "              |                    |\n"
    "              |                    v\n"
    "              |        |========================|\n"
    "              |--------|       VM: Loaded       |<-------|\n"
    "              |        |========================|        |\n"
    "              |              |            ^              |\n"
    "              |         Validate          |              |\n"
    "          Cleanup            |          LoadWasm         |\n"
    "              |              v            |            LoadWasm\n"
    "              |        |========================|        |\n"
    "              |--------|      VM: Validated     |        |\n"
    "              |        |========================|        |\n"
    "              |              |            ^              |\n"
    "              |      Instantiate          |              |\n"
    "              |              |          RegisterModule   |\n"
    "              |              v            |              |\n"
    "              |        |========================|        |\n"
    "              |--------|    VM: Instantiated    |--------|\n"
    "                       |========================|\n"
    "                             |            ^\n"
    "                             |            |\n"
    "                             --------------\n"
    "                Instantiate, Execute, ExecuteRegistered.\n"
    "The status of the VM context would be Inited when created. After loading "
    "WASM successfully, the status will be Loaded. After validating WASM "
    "successfully, the status will be Validated. After instantiating WASM "
    "successfully, the status will be Instantiated, and developers can invoke "
    "functions. Developers can register WASM or import objects in any status, "
    "but they should instantiate WASM again. Developers can also load WASM in "
    "any status, and they should validate and instantiate the WASM module "
    "before function invocation. When in the Instantiated status, developers "
    "can instantiate the WASM module again to reset the old WASM runtime "
    "structures.The VM creation API accepts the Configure context and the "
    "Store context. If developers only need the default settings, just pass "
    "NULL to the creation API. The details of the Store context will be "
    "introduced in Store.";

const char *logging_str =
    "The WasmEdge_LogSetErrorLevel() and "
    "WasmEdge_LogSetDebugLevel() APIs can set the logging "
    "system to debug level or error level. By default, the "
    "error level is set, and the debug info is hidden.";

const char *Configure_str =
    "The objects, such as VM, Store, and HostFunction, are composed of "
    "Contexts. All of the contexts can be created by calling the corresponding "
    "creation APIs and should be destroyed by calling the corresponding "
    "deletion APIs. Developers have responsibilities to manage the contexts "
    "for memory management.";

const char *result_str =
    "The WasmEdge_Result object specifies the execution status. APIs about "
    "WASM execution will return the WasmEdge_Result to denote the status.";

const char *Store_str =
    "Store is the runtime structure for the representation of all instances of "
    "Functions, Tables, Memorys, and Globals that have been allocated during "
    "the lifetime of the abstract machine. The Store context in WasmEdge "
    "provides APIs to list the exported instances with their names or find the "
    "instances by exported names. For adding instances into Store contexts, "
    "please instantiate or register WASM modules or Import Object contexts via "
    "the Interpreter context.\nIf the VM context is created without assigning "
    "a Store context, the VM context will allocate and own a Store "
    "context.Developers can also create the VM context with a Store context. "
    "In this case, developers should guarantee the life cycle of the Store "
    "context. ";

} // namespace pysdk

#endif // WASMEDGE_DOC_STRINGS_HPP