#include "WasmEdge.hpp"
#include <stdexcept>
#include <string>

/* --------------- VM -------------------------------- */
pysdk::VM::VM() { VMCxt = WasmEdge_VMCreate(NULL, NULL); }

pysdk::VM::VM(pysdk::Store &store) {
  VMCxt = WasmEdge_VMCreate(NULL, store.get());
}

pysdk::VM::VM(pysdk::Configure &cfg) {
  VMCxt = WasmEdge_VMCreate(cfg.get(), NULL);
}

pysdk::VM::VM(pysdk::Configure &cfg, pysdk::Store &store) {
  VMCxt = WasmEdge_VMCreate(cfg.get(), store.get());
}

pysdk::VM::~VM() { WasmEdge_VMDelete(VMCxt); }

/**
 * @brief Instantiate the WASM module from a WASM file and invoke a function by
 * name.This is the function to invoke a WASM function rapidly.Load and
 * instantiate the WASM module from the file path, and then invoke afunction by
 * name and parameters.
 *
 * @param FileName string of path
 * @param FuncName string of function name
 * @param param_list tuple(WasmEdge.Value)
 * @param ret_len Length of return buffer
 * @return pybind11::tuple of result,list of WasmEdge.Value
 */
pybind11::tuple pysdk::VM::run_from_wasm_file(std::string &FileName,
                                              std::string &FuncName,
                                              pybind11::tuple param_list,
                                              uint32_t &ret_len) {
  auto const param_len = pybind11::len(param_list);

  WasmEdge_Value Params[param_len];
  WasmEdge_Value Returns[ret_len];

  for (int i = 0; i < param_len; i++) {
    Params[i] = param_list[i].cast<pysdk::Value>().get();
  }

  WasmEdge_String funcName = WasmEdge_StringCreateByCString(FuncName.c_str());

  pysdk::result res(WasmEdge_VMRunWasmFromFile(
      VMCxt, FileName.c_str(), funcName, Params, param_len, Returns, ret_len));

  WasmEdge_StringDelete(funcName);

  pybind11::list returns;
  for (int i = 0; i < ret_len; i++) {
    returns.append(pysdk::Value(Returns[i]));
  }
  return pybind11::make_tuple(res, returns);
}

pysdk::Async pysdk::VM::run_from_wasm_file_async(std::string &FileName,
                                                 std::string &FuncName,
                                                 pybind11::tuple param_list) {
  auto const param_len = pybind11::len(param_list);

  WasmEdge_Value Params[param_len];

  for (int i = 0; i < param_len; i++) {
    Params[i] = param_list[i].cast<pysdk::Value>().get();
  }

  WasmEdge_String funcName = WasmEdge_StringCreateByCString(FuncName.c_str());

  pysdk::Async res(WasmEdge_VMAsyncRunWasmFromFile(
      VMCxt, FileName.c_str(), funcName, Params, param_len));

  WasmEdge_StringDelete(funcName);

  return res;
}

pybind11::tuple pysdk::VM::run_from_buffer(pybind11::tuple wasm_buffer,
                                           pybind11::tuple params_,
                                           std::string &executor_func_name,
                                           uint32_t &return_len) {

  pybind11::list returns;

  auto params = params_.cast<pybind11::tuple>();
  auto param_len = pybind11::len(params);
  auto size = wasm_buffer.size();
  uint8_t WASM_Buffer[size];

  for (size_t i = 0; i < size; i++) {
    WASM_Buffer[i] = wasm_buffer[i].cast<uint8_t>();
  }

  WasmEdge_Value Params[param_len];
  for (int i = 0; i < param_len; i++) {
    Params[i] = params[i].cast<pysdk::Value>().get();
  }

  WasmEdge_String ex_func_name_wasm =
      WasmEdge_StringCreateByCString(executor_func_name.c_str());

  WasmEdge_Value Returns[return_len];

  pysdk::result res(
      WasmEdge_VMRunWasmFromBuffer(VMCxt, WASM_Buffer, size, ex_func_name_wasm,
                                   Params, param_len, Returns, return_len));

  WasmEdge_StringDelete(ex_func_name_wasm);

  for (int i = 0; i < return_len; i++) {
    returns.append(pysdk::Value(Returns[i]));
  }

  return pybind11::make_tuple(res, returns);
}

pysdk::Async pysdk::VM::run_from_buffer_async(pybind11::tuple wasm_buffer,
                                              pybind11::tuple params_,
                                              std::string &executor_func_name) {

  auto params = params_.cast<pybind11::tuple>();
  auto param_len = pybind11::len(params);
  auto size = wasm_buffer.size();
  uint8_t WASM_Buffer[size];

  for (size_t i = 0; i < size; i++) {
    WASM_Buffer[i] = wasm_buffer[i].cast<uint8_t>();
  }

  WasmEdge_Value Params[param_len];
  for (int i = 0; i < param_len; i++) {
    Params[i] = params[i].cast<pysdk::Value>().get();
  }

  WasmEdge_String ex_func_name_wasm =
      WasmEdge_StringCreateByCString(executor_func_name.c_str());

  pysdk::Async res(WasmEdge_VMAsyncRunWasmFromBuffer(
      VMCxt, WASM_Buffer, size, ex_func_name_wasm, Params, param_len));

  WasmEdge_StringDelete(ex_func_name_wasm);

  return res;
}

pysdk::result pysdk::VM::register_module_from_file(std::string &mod_name_,
                                                   std::string &path) {
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod_name_.c_str());
  pysdk::result res(
      WasmEdge_VMRegisterModuleFromFile(VMCxt, mod_name, path.c_str()));
  WasmEdge_StringDelete(mod_name);
  return res;
}

pysdk::result pysdk::VM::register_module_from_ast(std::string &mod_name_,
                                                  pysdk::ASTModuleCxt &ast) {
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod_name_.c_str());
  pysdk::result res(
      WasmEdge_VMRegisterModuleFromASTModule(VMCxt, mod_name, ast.get()));
  WasmEdge_StringDelete(mod_name);
  return res;
}

pysdk::result pysdk::VM::register_module_from_buffer(std::string &mod_name_,
                                                     pybind11::tuple buffer) {
  auto len = pybind11::len(buffer);
  uint8_t buffer_[len];
  for (size_t i = 0; i < len; i++) {
    buffer_[i] = buffer[i].cast<uint8_t>();
  }
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod_name_.c_str());
  pysdk::result res(
      WasmEdge_VMRegisterModuleFromBuffer(VMCxt, mod_name, buffer_, len));
  WasmEdge_StringDelete(mod_name);
  return res;
}

pysdk::result
pysdk::VM::register_module_from_import_object(pysdk::import_object &imp_obj) {
  pysdk::result res(WasmEdge_VMRegisterModuleFromImport(
      VMCxt, const_cast<const WasmEdge_ImportObjectContext *>(imp_obj.get())));
  return res;
}

/**
 * @brief Execute WASM functions in registered modules.
 * Unlike the execution of functions, the registered functions can be invoked
 * without `WasmEdge_VMInstantiate()` because the WASM module was instantiated
 * when registering. Developers can also invoke the host functions directly with
 * this API.
 *
 * @param mod_name  Name of the Module
 * @param func_name Name of the function
 * @param params  `list` of `WasmEdge.Value(s)`
 * @param ReturnLen length of return values
 * @return pybind11::tuple
 */
pybind11::tuple pysdk::VM::execute_registered(std::string &mod_name,
                                              std::string &func_name,
                                              pybind11::list params,
                                              const uint32_t &ReturnLen) {
  const WasmEdge_String ModuleName =
      WasmEdge_StringCreateByCString(mod_name.c_str());
  const WasmEdge_String FuncName =
      WasmEdge_StringCreateByCString(func_name.c_str());
  const uint32_t ParamLen = pybind11::len(params);
  WasmEdge_Value Params[ParamLen];

  for (size_t i = 0; i < ParamLen; i++) {
    Params[i] = params[i].cast<pysdk::Value>().get();
  }

  WasmEdge_Value Returns[ReturnLen];

  pysdk::result res(WasmEdge_VMExecuteRegistered(
      VMCxt, ModuleName, FuncName, const_cast<const WasmEdge_Value *>(Params),
      ParamLen, Returns, ReturnLen));

  pybind11::list ret;
  for (size_t i = 0; i < ReturnLen; i++) {
    ret.append(pysdk::Value(Returns[i]));
  }

  return pybind11::make_tuple(res, ret);
}

pybind11::dict pysdk::VM::get_functions(uint32_t &len) {
  pybind11::dict dict;
  WasmEdge_String Names[len];
  const WasmEdge_FunctionTypeContext *FuncTypes[len];
  const auto len_api = WasmEdge_VMGetFunctionList(VMCxt, Names, FuncTypes, len);

  for (size_t i = 0; i < len_api; i++) {
    char buf[Names[i].Length];
    WasmEdge_StringCopy(Names[i], buf, Names[i].Length);
    dict[pybind11::str(std::string(buf))] = pysdk::FunctionTypeContext(
        const_cast<WasmEdge_FunctionTypeContext *>(FuncTypes[i]));
  }
  return dict;
}

uint32_t pysdk::VM::get_functions_len() {
  return WasmEdge_VMGetFunctionListLength(VMCxt);
}

pysdk::FunctionTypeContext pysdk::VM::get_function_type(std::string &name) {
  WasmEdge_String str = WasmEdge_StringCreateByCString(name.c_str());
  pysdk::FunctionTypeContext cxt(const_cast<WasmEdge_FunctionTypeContext *>(
      WasmEdge_VMGetFunctionType(VMCxt, str)));
  WasmEdge_StringDelete(str);
  return cxt;
}

pysdk::FunctionTypeContext
pysdk::VM::get_function_type_registered(std::string &mod_name,
                                        std::string &name) {
  WasmEdge_String mod = WasmEdge_StringCreateByCString(mod_name.c_str());
  WasmEdge_String func = WasmEdge_StringCreateByCString(name.c_str());
  pysdk::FunctionTypeContext cxt(const_cast<WasmEdge_FunctionTypeContext *>(
      WasmEdge_VMGetFunctionTypeRegistered(VMCxt, mod, func)));
  WasmEdge_StringDelete(mod);
  WasmEdge_StringDelete(func);
  return cxt;
}

pysdk::import_object
pysdk::VM::get_import_module_context(WasmEdge_HostRegistration &reg) {
  return pysdk::import_object(WasmEdge_VMGetImportModuleContext(VMCxt, reg));
}

pysdk::Statistics pysdk::VM::get_statistics_context() {
  return pysdk::Statistics(WasmEdge_VMGetStatisticsContext(VMCxt));
}

pysdk::Store pysdk::VM::get_store_cxt() {
  return pysdk::Store(WasmEdge_VMGetStoreContext(VMCxt));
}

pysdk::result pysdk::VM::instantiate() {
  return pysdk::result(WasmEdge_VMInstantiate(VMCxt));
}

pysdk::result pysdk::VM::load_from_ast(pysdk::ASTModuleCxt &ast) {
  return pysdk::result(WasmEdge_VMLoadWasmFromASTModule(
      VMCxt, const_cast<const WasmEdge_ASTModuleContext *>(ast.get())));
}

pysdk::result pysdk::VM::load_from_buffer(pybind11::tuple tup) {
  auto const len = pybind11::len(tup);
  uint8_t buf[len];
  for (size_t i = 0; i < len; i++) {
    buf[i] = tup[i].cast<uint8_t>();
  }
  return pysdk::result(WasmEdge_VMLoadWasmFromBuffer(
      VMCxt, const_cast<const uint8_t *>(buf), len));
}

pysdk::result pysdk::VM::load_from_file(std::string &path) {
  return pysdk::result(WasmEdge_VMLoadWasmFromFile(VMCxt, path.c_str()));
}

pybind11::tuple pysdk::VM::run_from_ast(pysdk::ASTModuleCxt &cxt,
                                        std::string &function_name,
                                        pybind11::tuple params,
                                        uint32_t &ret_len) {
  WasmEdge_String func_name =
      WasmEdge_StringCreateByCString(function_name.c_str());
  auto const param_len = pybind11::len(params);
  WasmEdge_Value param[param_len];

  for (size_t i = 0; i < param_len; i++) {
    param[i] = params[i].cast<pysdk::Value>().get();
  }
  WasmEdge_Value ret[ret_len];

  pysdk::result res(WasmEdge_VMRunWasmFromASTModule(
      VMCxt, cxt.get(), func_name, param, param_len, ret, ret_len));
  pybind11::list list;

  for (size_t i = 0; i < ret_len; i++) {
    list.append(pysdk::Value(ret[i]));
  }
  return pybind11::make_tuple(res, list);
}

pysdk::Async pysdk::VM::run_from_ast_async(pysdk::ASTModuleCxt &cxt,
                                           std::string &function_name,
                                           pybind11::tuple params) {
  WasmEdge_String func_name =
      WasmEdge_StringCreateByCString(function_name.c_str());
  auto const param_len = pybind11::len(params);
  WasmEdge_Value param[param_len];

  for (size_t i = 0; i < param_len; i++) {
    param[i] = params[i].cast<pysdk::Value>().get();
  }

  return pysdk::Async(WasmEdge_VMAsyncRunWasmFromASTModule(
      VMCxt, cxt.get(), func_name, param, param_len));
}

pybind11::tuple pysdk::VM::execute(std::string &function_name,
                                   pybind11::tuple params, uint32_t &ret_len) {
  WasmEdge_String func_name =
      WasmEdge_StringCreateByCString(function_name.c_str());
  auto const param_len = pybind11::len(params);
  WasmEdge_Value param[param_len];

  for (size_t i = 0; i < param_len; i++) {
    param[i] = params[i].cast<pysdk::Value>().get();
  }
  WasmEdge_Value ret[ret_len];

  pysdk::result res(
      WasmEdge_VMExecute(VMCxt, func_name, param, param_len, ret, ret_len));
  pybind11::list list;

  for (size_t i = 0; i < ret_len; i++) {
    list.append(pysdk::Value(ret[i]));
  }
  return pybind11::make_tuple(res, list);
}

pysdk::Async pysdk::VM::executeAsync(std::string &function_name,
                                     pybind11::tuple params) {
  WasmEdge_String func_name =
      WasmEdge_StringCreateByCString(function_name.c_str());
  auto const param_len = pybind11::len(params);
  WasmEdge_Value param[param_len];

  for (size_t i = 0; i < param_len; i++) {
    param[i] = params[i].cast<pysdk::Value>().get();
  }

  return pysdk::Async(
      WasmEdge_VMAsyncExecute(VMCxt, func_name, param, param_len));
}

pysdk::Async pysdk::VM::executeAsyncRegistered(std::string &mod,
                                               std::string &function_name,
                                               pybind11::tuple params) {
  WasmEdge_String func_name =
      WasmEdge_StringCreateByCString(function_name.c_str());
  WasmEdge_String mod_name = WasmEdge_StringCreateByCString(mod.c_str());

  auto const param_len = pybind11::len(params);
  WasmEdge_Value param[param_len];

  for (size_t i = 0; i < param_len; i++) {
    param[i] = params[i].cast<pysdk::Value>().get();
  }

  return pysdk::Async(WasmEdge_VMAsyncExecuteRegistered(
      VMCxt, mod_name, func_name, param, param_len));
}

pysdk::result pysdk::VM::validate() {
  return pysdk::result(WasmEdge_VMValidate(VMCxt));
}
/* --------------- VM End -------------------------------- */
