#include <iostream>
#include <string>
#include <vector>
#include <wasmedge/wasmedge.hh>

int main(int Argc, const char *Argv[]) {
  /* Create the configure context and add the WASI support. */
  /* This step is not necessary unless you need WASI support. */
  WasmEdge::Configuration ConfigCxt();
  ConfigCxt.AddHostRegistration(WasmEdge::HostRegistration::Wasi);

  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge::VM VMCxt(ConfigCxt);

  /* The parameters and returns vectors. */
  std::vector<WasmEdge::Value> Params;
  Params.push_back(WasmEdge::Value(32));
  std::vector<WasmEdge::Value> Returns;

  /* Function name. */
  std::string FuncName = "fib";
  std::string Path(Argv[1]);
  WasmEdge::Result Res = VMCxt.RunWasm(Path, FuncName, Params, Returns);

  if (Res.IsOk()) {
    std::cout << "Get Result: " << Returns.at(0).GetI32() << std::endl;
  } else {
    std::cout << "Error Message: " << Res.GetMessage() << std::endl;
  }

  return 0;
}