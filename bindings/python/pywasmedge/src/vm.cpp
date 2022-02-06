#include "WasmEdge.hpp"
#include <stdexcept>
#include <string>

/* --------------- VM -------------------------------- */
pysdk::VM::VM() { VMCxt = WasmEdge_VMCreate(NULL, NULL); }

pysdk::VM::VM(pysdk::Store &store) {

  // ;
  // WasmEdge_VMLoadWasmFromFile();
  // WasmEdge_VMRegisterModuleFromASTModule();
  // WasmEdge_VMRegisterModuleFromBuffer();
  // WasmEdge_VMRegisterModuleFromFile();
  // WasmEdge_VMRegisterModuleFromImport();
  // WasmEdge_VMRunWasmFromASTModule();
  // WasmEdge_VMRunWasmFromBuffer();
  // WasmEdge_VMRunWasmFromFile();
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
 * @brief Execute VM
 *
 * @param _FileName Name of .wasm binary
 * @param _FuncName Wasm Function name
 * @param _params Wasm Function Parameters
 * @param _param_types List of `Type` of parameters
 * @param _ret_types List of `Type` of Return values. This length must be same
 * as return length of wasm function.
 * @return pybind11::tuple
 */
pybind11::tuple pysdk::VM::run(pybind11::object _FileName,
                               pybind11::object _FuncName,
                               pybind11::object _params,
                               pybind11::object _param_types,
                               pybind11::object _ret_types) {
  std::string FileName = _FileName.cast<std::string>();
  std::string FuncName = _FuncName.cast<std::string>();
  const int ret_len = pybind11::len(_ret_types);

  auto param_len = pybind11::len(_params);
  auto param_list = _params.cast<pybind11::list>();
  auto _param_types_list = _param_types.cast<pybind11::list>();
  auto _ret_types_list = _ret_types.cast<pybind11::list>();

  WasmEdge_Value Params[param_len];
  WasmEdge_Value Returns[ret_len];
  for (int i = 0; i < param_len; i++) {
    switch (_param_types_list[i].cast<WasmEdge_ValType>()) {
    case WasmEdge_ValType_I32:
      Params[i] = WasmEdge_ValueGenI32(param_list[i].cast<int32_t>());
      break;
    case WasmEdge_ValType_I64:
      Params[i] = WasmEdge_ValueGenI64(param_list[i].cast<int64_t>());
      break;
    case WasmEdge_ValType_F32:
      Params[i] = WasmEdge_ValueGenF32(param_list[i].cast<float>());
      break;
    case WasmEdge_ValType_F64:
      Params[i] = WasmEdge_ValueGenF32(param_list[i].cast<double>());
      break;
    case WasmEdge_ValType_V128:
      Params[i] = WasmEdge_ValueGenV128(param_list[i].cast<int128_t>());
      break;
    case WasmEdge_ValType_FuncRef:
      Params[i] = WasmEdge_ValueGenFuncRef(param_list[i].cast<uint32_t>());
      break;
    // TODO: Handle Pointer
    // case WasmEdge_ValType_ExternRef:
    //   Params[i] = WasmEdge_ValueGenExternRef(
    //       param_list[i].cast<(void *)>());
    //   break;
    default:
      break;
    }
  }

  WasmEdge_String funcName{(uint32_t)FuncName.length(), FuncName.c_str()};
  pysdk::result res(WasmEdge_VMRunWasmFromFile(
      VMCxt, FileName.c_str(), funcName, Params, param_len, Returns, ret_len));

  pybind11::list returns;
  for (int i = 0; i < ret_len; i++) {
    switch (_ret_types_list[i].cast<WasmEdge_ValType>()) {
    case WasmEdge_ValType_I32:
      returns.append(pybind11::cast(WasmEdge_ValueGetI32(Returns[i])));
      break;
    case WasmEdge_ValType_I64:
      returns.append(pybind11::cast(WasmEdge_ValueGetI64(Returns[i])));
      break;
    case WasmEdge_ValType_F32:
      returns.append(pybind11::cast(WasmEdge_ValueGetF32(Returns[i])));
      break;
    case WasmEdge_ValType_F64:
      returns.append(pybind11::cast(WasmEdge_ValueGetF64(Returns[i])));
      break;
    case WasmEdge_ValType_V128:
      returns.append(pybind11::cast(WasmEdge_ValueGetV128(Returns[i])));
      break;
    case WasmEdge_ValType_FuncRef:
      returns.append(pybind11::cast(WasmEdge_ValueGetFuncIdx(Returns[i])));
      break;
    // TODO: Handle Void Pointer
    // case WasmEdge_ValType_ExternRef:
    //   returns.append(pybind11::cast(WasmEdge_ValueGetExternRef(Returns[i])));
    //   break;
    default:
      break;
    }
  }
  return pybind11::make_tuple(res, returns);
}

/**
 * @brief Execute VM without specifying parameter, return types and length.
 * Executes VM step by step.
 *
 * @param _FileName Name of .wasm binary
 * @param _FuncName Wasm Function name
 * @param _params Wasm Function Parameters
 * @return pybind11::tuple
 */
pybind11::tuple pysdk::VM::run(pybind11::object _FileName,
                               pybind11::object _FuncName,
                               pybind11::object _params) {
  pybind11::list returns;

  std::string FileName = _FileName.cast<std::string>();
  std::string FuncName = _FuncName.cast<std::string>();

  pysdk::result res(WasmEdge_VMLoadWasmFromFile(VMCxt, FileName.c_str()));
  if (!res) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(res, NULL);
  }

  res = WasmEdge_VMValidate(VMCxt);
  if (!res) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(res, NULL);
  }

  res = WasmEdge_VMInstantiate(VMCxt);
  if (!res) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(res, NULL);
  }

  WasmEdge_FunctionTypeContext *FuncTypeCxt =
      (WasmEdge_FunctionTypeContext *)WasmEdge_VMGetFunctionType(
          VMCxt, WasmEdge_StringCreateByCString(FuncName.c_str()));

  auto params_list = _params.cast<pybind11::list>();
  auto param_len = pybind11::len(_params);
  auto param_len_api = WasmEdge_FunctionTypeGetParametersLength(FuncTypeCxt);

  if (param_len != param_len_api) {
    /* TODO: Handle errors gracefully */
    throw std::runtime_error(
        "Received Unmatched parameter length: " + std::to_string(param_len) +
        ", API->" + std::to_string(param_len_api));
    return pybind11::make_tuple(NULL, NULL);
  }

  WasmEdge_ValType val_type_list_param[param_len];
  WasmEdge_FunctionTypeGetParameters(FuncTypeCxt, val_type_list_param,
                                     param_len);
  WasmEdge_Value Params[param_len];
  for (int i = 0; i < param_len; i++) {
    switch (val_type_list_param[i]) {
    case WasmEdge_ValType_I32:
      Params[i] = WasmEdge_ValueGenI32(params_list[i].cast<int32_t>());
      break;
    case WasmEdge_ValType_I64:
      Params[i] = WasmEdge_ValueGenI64(params_list[i].cast<int64_t>());
      break;
    case WasmEdge_ValType_F32:
      Params[i] = WasmEdge_ValueGenF32(params_list[i].cast<float>());
      break;
    case WasmEdge_ValType_F64:
      Params[i] = WasmEdge_ValueGenF32(params_list[i].cast<double>());
      break;
    case WasmEdge_ValType_V128:
      Params[i] = WasmEdge_ValueGenV128(params_list[i].cast<int128_t>());
      break;
    case WasmEdge_ValType_FuncRef:
      Params[i] = WasmEdge_ValueGenFuncRef(params_list[i].cast<uint32_t>());
      break;
    // TODO: Handle Pointer
    // case WasmEdge_ValType_ExternRef:
    //   Params[i] = WasmEdge_ValueGenExternRef(
    //       params_list[i].cast<(void *)>());
    //   break;
    default:
      break;
    }
  }

  auto ret_len = WasmEdge_FunctionTypeGetReturnsLength(FuncTypeCxt);
  WasmEdge_ValType val_type_list_ret[ret_len];
  if (ret_len != WasmEdge_FunctionTypeGetReturns(FuncTypeCxt, val_type_list_ret,
                                                 ret_len)) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(NULL, NULL);
  };

  WasmEdge_Value Returns[ret_len];
  WasmEdge_String funcName{(uint32_t)FuncName.length(), FuncName.c_str()};

  res =
      WasmEdge_VMExecute(VMCxt, funcName, Params, param_len, Returns, ret_len);

  for (int i = 0; i < ret_len; i++) {
    switch (val_type_list_ret[i]) {
    case WasmEdge_ValType_I32:
      returns.append(pybind11::cast(WasmEdge_ValueGetI32(Returns[i])));
      break;
    case WasmEdge_ValType_I64:
      returns.append(pybind11::cast(WasmEdge_ValueGetI64(Returns[i])));
      break;
    case WasmEdge_ValType_F32:
      returns.append(pybind11::cast(WasmEdge_ValueGetF32(Returns[i])));
      break;
    case WasmEdge_ValType_F64:
      returns.append(pybind11::cast(WasmEdge_ValueGetF64(Returns[i])));
      break;
    case WasmEdge_ValType_V128:
      returns.append(pybind11::cast(WasmEdge_ValueGetV128(Returns[i])));
      break;
    case WasmEdge_ValType_FuncRef:
      returns.append(pybind11::cast(WasmEdge_ValueGetFuncIdx(Returns[i])));
      break;
    // TODO: Handle Void Pointer
    case WasmEdge_ValType_ExternRef:
      returns.append(pybind11::cast(WasmEdge_ValueGetExternRef(Returns[i])));
      break;
    default:
      break;
    }
  }

  return pybind11::make_tuple(res, returns);
}

pybind11::tuple pysdk::VM::run(pybind11::object wasm_buffer_,
                               pybind11::object params_,
                               pybind11::object return_len_,
                               std::string &executor_func_name) {

  auto return_len = return_len_.cast<int>();

  auto wasm_buffer = wasm_buffer_.cast<pybind11::tuple>();
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

pysdk::StatisticsContext pysdk::VM::get_statistics_context() {
  return pysdk::StatisticsContext(WasmEdge_VMGetStatisticsContext(VMCxt));
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
/* --------------- VM End -------------------------------- */
