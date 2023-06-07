#include <iostream>
#include <string>
#include <vector>
#include <wasmedge/wasmedge.hh>

int main(int Argc, const char *Argv[]) {
  /*
   * Create the configure context with default values.
   */
  WasmEdge::Configuration ConfCxt();

  /*
   * Create the statistics context. This step is not necessary if the statistics
   * in runtime is not needed.
   */
  WasmEdge::Statistics StatCxt = WasmEdge::Statistics::New();

  /*
   * Create the store context. The store context is the object to link the
   * modules for imports and exports.
   */
  WasmEdge::Store StoreCxt = WasmEdge::Store::New();
  WasmEdge::Result *Res;

  /* Create the loader context. */
  WasmEdge::Loader LoadCxt = WasmEdge::Loader::New(ConfCxt);
  /* Create the validator context. */
  WasmEdge::Validator ValidCxt = WasmEdge::Validator::New(ConfCxt);
  /* Create the executor context. */
  WasmEdge::Executor ExecCxt = WasmEdge::Executor::New(ConfCxt, StatCxt);

  /*
   * Load the WASM file or the compiled-WASM file and convert into the AST
   * module context.
   */
  WasmEdge::ASTModule ASTCxt = WasmEdge::ASTModule::New();
  std::string Path(Argv[1]);
  *Res = LoadCxt.Parse(ASTCxt, Path);
  if (Res->IsOk()) {
    std::cout << "Loading phase failed:" << Res->GetMessage() << std::endl;
    return 1;
  }
  /* Validate the WASM module. */
  *Res = ValidCxt.Validate(ASTCxt);
  if (Res->IsOk()) {
    std::cout << "Validation phase failed:" << Res->GetMessage() << std::endl;
    return 1;
  }
  /* Instantiate the WASM module into store context. */
  WasmEdge::ModuleInstance ModInst = WasmEdge::ModuleInstance::New();
  *Res = ExecCxt.Instantiate(ModInst, StoreCxt, ASTCxt);
  if (Res->IsOk()) {
    std::cout << "Instantiation phase failed:" << Res->GetMessage()
              << std::endl;
    return 1;
  }
  /* Try to list the exported functions of the instantiated WASM module. */
  std::vector<std::string> FuncList = ModInst.ListFunction();
  std::cout << "Total number of exported functions: " << FuncList.size()
            << std::endl;
  for (auto &FuncName : FuncList) {
    std::cout << "Get exported function name: " << FuncName << std::endl;
  }

  /* The parameters and returns vectors. */
  std::vector<WasmEdge::Value> Params;
  Params.push_back(WasmEdge::Value(32));
  std::vector<WasmEdge::Value> Returns;

  /* Function name. */
  std::string FuncName = "fib";
  /* Find the exported function by function name. */
  WasmEdge::FunctionInstance FuncInst = ModInst.FindFunction(FuncName);
  if (!FuncInst) {
    std::cout << "Function `fib` not found." << std::endl;
    return 1;
  }

  /* Invoke the WASM function. */
  *Res = ExecCxt.Invoke(FuncInst, Params, Returns);
  if (Res.IsOk()) {
    std::cout << "Get Result: " << Returns.at(0).GetI32() << std::endl;
  } else {
    std::cout << "Execution phase failed: " << Res.GetMessage() << std::endl;
  }

  return 0;
}