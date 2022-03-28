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

pybind11::list pysdk::Store::ListFunction(uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String FuncNames[len];

  auto GotFuncNum = WasmEdge_StoreListFunction(StoreCxt, FuncNames, len);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char temp[FuncNames[I].Length];
    WasmEdge_StringCopy(FuncNames[I], temp, FuncNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

uint32_t pysdk::Store::ListFunctionLength() {
  return WasmEdge_StoreListFunctionLength(StoreCxt);
}

uint32_t pysdk::Store::ListFunctionRegisteredLength(std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  auto ret = WasmEdge_StoreListFunctionRegisteredLength(StoreCxt, name);
  WasmEdge_StringDelete(name);
  return ret;
}

pybind11::list pysdk::Store::ListModule(uint32_t &len) {
  pybind11::list ret;

  WasmEdge_String ModNames[len];

  auto GotFuncNum = WasmEdge_StoreListModule(StoreCxt, ModNames, len);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char temp[ModNames[I].Length];
    WasmEdge_StringCopy(ModNames[I], temp, ModNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

pybind11::list
pysdk::Store::ListFunctionRegistered(const std::string &module_name,
                                     uint32_t &len) {
  pybind11::list ret;

  WasmEdge_String module_name_wasm =
      WasmEdge_StringCreateByCString(module_name.c_str());

  WasmEdge_String RegFuncNames[len];
  auto GotFuncNum = WasmEdge_StoreListFunctionRegistered(
      StoreCxt, module_name_wasm, RegFuncNames, len);
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

pysdk::Function pysdk::Store::FindFunction(std::string &name) {
  WasmEdge_String fname = WasmEdge_StringCreateByCString(name.c_str());
  pysdk::Function f(WasmEdge_StoreFindFunction(StoreCxt, fname));
  WasmEdge_StringDelete(fname);
  return f;
}

pysdk::Function pysdk::Store::FindFunctionRegistered(std::string &mod,
                                                     std::string &func) {
  WasmEdge_String fname = WasmEdge_StringCreateByCString(func.c_str());
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  pysdk::Function f(
      WasmEdge_StoreFindFunctionRegistered(StoreCxt, mod_name, fname));
  WasmEdge_StringDelete(fname);
  WasmEdge_StringDelete(mod_name);
  return f;
}

pysdk::Global pysdk::Store::FindGlobal(std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  pysdk::Global gl(WasmEdge_StoreFindGlobal(StoreCxt, name));
  WasmEdge_StringDelete(name);
  return gl;
}

pysdk::Global pysdk::Store::FindGlobalRegistered(std::string &mod,
                                                 std::string &func) {
  WasmEdge_String module_name = WasmEdge_StringCreateByCString(mod.c_str());
  WasmEdge_String func_name = WasmEdge_StringCreateByCString(func.c_str());
  pysdk::Global gl(
      WasmEdge_StoreFindGlobalRegistered(StoreCxt, module_name, func_name));
  WasmEdge_StringDelete(module_name);
  WasmEdge_StringDelete(func_name);
  return gl;
}

pysdk::Memory pysdk::Store::FindMemoryRegistered(std::string &mod,
                                                 std::string &str) {
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  WasmEdge_String mem_name = WasmEdge_StringCreateByCString(str.c_str());
  pysdk::Memory m(
      WasmEdge_StoreFindMemoryRegistered(StoreCxt, mod_name, mem_name));
  WasmEdge_StringDelete(mod_name);
  WasmEdge_StringDelete(mem_name);
  return m;
}

pysdk::Memory pysdk::Store::FindMemory(std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  pysdk::Memory m(WasmEdge_StoreFindMemory(StoreCxt, name));
  WasmEdge_StringDelete(name);
  return m;
}

pysdk::Table pysdk::Store::FindTable(std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  pysdk::Table t(WasmEdge_StoreFindTable(StoreCxt, name), false);
  WasmEdge_StringDelete(name);
  return t;
}

pysdk::Table pysdk::Store::FindTableRegistered(std::string &mod,
                                               std::string &tab) {
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  WasmEdge_String tab_name = WasmEdge_StringCreateByCString(tab.c_str());
  pysdk::Table t(
      WasmEdge_StoreFindTableRegistered(StoreCxt, mod_name, tab_name), false);
  WasmEdge_StringDelete(mod_name);
  WasmEdge_StringDelete(tab_name);
  return t;
}

pybind11::list pysdk::Store::ListGlobal(uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String GlobNames[len];

  auto GotGlobNum = WasmEdge_StoreListGlobal(StoreCxt, GlobNames, len);
  for (uint32_t I = 0; I < GotGlobNum; I++) {
    char temp[GlobNames[I].Length];
    WasmEdge_StringCopy(GlobNames[I], temp, GlobNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

pybind11::list pysdk::Store::ListGlobalRegistered(std::string &mod,
                                                  uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String GlobNames[len];
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  auto GotGlobNum =
      WasmEdge_StoreListGlobalRegistered(StoreCxt, mod_name, GlobNames, len);
  for (uint32_t I = 0; I < GotGlobNum; I++) {
    char temp[GlobNames[I].Length];
    WasmEdge_StringCopy(GlobNames[I], temp, GlobNames[I].Length);
    ret.append(std::string(temp));
  }
  WasmEdge_StringDelete(mod_name);
  return ret;
}

uint32_t pysdk::Store::ListGlobalLength() {
  return WasmEdge_StoreListGlobalLength(StoreCxt);
}

uint32_t pysdk::Store::ListGlobalRegisteredLength(std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  auto ret = WasmEdge_StoreListGlobalRegisteredLength(StoreCxt, name);
  WasmEdge_StringDelete(name);
  return ret;
}

pybind11::list pysdk::Store::ListMemory(uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String MemNames[len];

  auto GotGlobNum = WasmEdge_StoreListMemory(StoreCxt, MemNames, len);
  for (uint32_t I = 0; I < GotGlobNum; I++) {
    char temp[MemNames[I].Length];
    WasmEdge_StringCopy(MemNames[I], temp, MemNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

pybind11::list pysdk::Store::ListMemoryRegistered(std::string &mod,
                                                  uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String MemNames[len];
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  auto GotGlobNum =
      WasmEdge_StoreListGlobalRegistered(StoreCxt, mod_name, MemNames, len);
  for (uint32_t I = 0; I < GotGlobNum; I++) {
    char temp[MemNames[I].Length];
    WasmEdge_StringCopy(MemNames[I], temp, MemNames[I].Length);
    ret.append(std::string(temp));
  }
  WasmEdge_StringDelete(mod_name);
  return ret;
}

uint32_t pysdk::Store::ListMemoryLength() {
  return WasmEdge_StoreListMemoryLength(StoreCxt);
}

uint32_t pysdk::Store::ListMemoryRegisteredLength(std::string &mod) {
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  auto ret = WasmEdge_StoreListMemoryRegisteredLength(StoreCxt, mod_name);
  WasmEdge_StringDelete(mod_name);
  return ret;
}

uint32_t pysdk::Store::ListModuleLength() {
  return WasmEdge_StoreListModuleLength(StoreCxt);
}

pybind11::list pysdk::Store::ListTable(uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String TabNames[len];

  auto GotGlobNum = WasmEdge_StoreListTable(StoreCxt, TabNames, len);
  for (uint32_t I = 0; I < GotGlobNum; I++) {
    char temp[TabNames[I].Length];
    WasmEdge_StringCopy(TabNames[I], temp, TabNames[I].Length);
    ret.append(std::string(temp));
  }

  return ret;
}

pybind11::list pysdk::Store::ListTableRegistered(std::string &mod,
                                                 uint32_t &len) {
  pybind11::list ret;
  WasmEdge_String TabNames[len];
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());
  auto GotGlobNum =
      WasmEdge_StoreListTableRegistered(StoreCxt, mod_name, TabNames, len);
  for (uint32_t I = 0; I < GotGlobNum; I++) {
    char temp[TabNames[I].Length];
    WasmEdge_StringCopy(TabNames[I], temp, TabNames[I].Length);
    ret.append(std::string(temp));
  }
  WasmEdge_StringDelete(mod_name);
  return ret;
}

uint32_t pysdk::Store::ListTableLength() {
  return WasmEdge_StoreListTableLength(StoreCxt);
}

uint32_t pysdk::Store::ListTableRegisteredLength(std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  auto ret = WasmEdge_StoreListTableRegisteredLength(StoreCxt, name);
  WasmEdge_StringDelete(name);
  return ret;
}
/* --------------- Store End -------------------------------- */
