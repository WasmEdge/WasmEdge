#include "WasmEdge.hpp"

/* --------------- Store -------------------------------- */

pysdk::Store::Store() { StoreCxt = WasmEdge_StoreCreate(); }

pysdk::Store::Store(WasmEdge_StoreContext *cxt) {
  StoreCxt = cxt;
  external = true;
}

pysdk::Store::~Store() {
  if (!external)
    WasmEdge_StoreDelete(StoreCxt);
}

WasmEdge_StoreContext *pysdk::Store::get() { return this->StoreCxt; }

pybind11::list pysdk::Store::listFunctions() {
  pybind11::list ret;

  auto FuncNum = WasmEdge_StoreListFunctionLength(StoreCxt);
  WasmEdge_String FuncNames[FuncNum];

  auto GotFuncNum = WasmEdge_StoreListFunction(StoreCxt, FuncNames, FuncNum);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char temp[FuncNames[I].Length];
    WasmEdge_StringCopy(FuncNames[I], temp, FuncNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

pybind11::list pysdk::Store::listModules() {
  pybind11::list ret;

  auto ModNum = WasmEdge_StoreListModuleLength(StoreCxt);
  WasmEdge_String ModNames[ModNum];

  auto GotFuncNum = WasmEdge_StoreListModule(StoreCxt, ModNames, ModNum);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char temp[ModNames[I].Length];
    WasmEdge_StringCopy(ModNames[I], temp, ModNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

pybind11::list
pysdk::Store::listRegisteredFunctions(const std::string &module_name) {
  pybind11::list ret;

  WasmEdge_String module_name_wasm =
      WasmEdge_StringCreateByCString(module_name.c_str());

  auto reg_func_length =
      WasmEdge_StoreListFunctionRegisteredLength(StoreCxt, module_name_wasm);
  WasmEdge_String RegFuncNames[reg_func_length];
  auto GotFuncNum = WasmEdge_StoreListFunctionRegistered(
      StoreCxt, module_name_wasm, RegFuncNames, reg_func_length);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char temp[RegFuncNames[I].Length];
    std::string temp_str;
    auto size =
        WasmEdge_StringCopy(RegFuncNames[I], temp, RegFuncNames[I].Length);
    for (size_t i = 0; i < size; i++) {
      temp_str += temp[i];
    }
    ret.append(temp_str);
  }

  return ret;
}

/* --------------- Store End -------------------------------- */
