#include "WasmEdge.hpp"

/* --------------- Executor-------------------------------- */
pysdk::Executor::Executor(pysdk::Configure &conf) {
  context = WasmEdge_ExecutorCreate(conf.get(), NULL);
}

pysdk::Executor::~Executor() {
  if (_del)
    WasmEdge_ExecutorDelete(context);
}

pysdk::result pysdk::Executor::instantiate(pysdk::Store &st,
                                           pysdk::ASTModuleCxt &ast) {

  return pysdk::result(
      WasmEdge_ExecutorInstantiate(context, st.get(), ast.get()));
}

pybind11::tuple pysdk::Executor::invoke(pysdk::Store &st, std::string &FuncName,
                                        pybind11::list _params) {

  auto store_list_len = WasmEdge_StoreListFunctionLength(st.get());

  WasmEdge_String funcName{(uint32_t)FuncName.length(), FuncName.c_str()};

  WasmEdge_FunctionInstanceContext *funcInst =
      WasmEdge_StoreFindFunction(st.get(), funcName);
  const WasmEdge_FunctionTypeContext *funcTypeCxt =
      WasmEdge_FunctionInstanceGetFunctionType(funcInst);

  auto param_len = WasmEdge_FunctionTypeGetParametersLength(funcTypeCxt);
  WasmEdge_ValType val_type_list_param[param_len];
  param_len = WasmEdge_FunctionTypeGetParameters(
      funcTypeCxt, val_type_list_param, param_len);
  auto params_list = _params.cast<pybind11::list>();
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

  auto ret_len = WasmEdge_FunctionTypeGetReturnsLength(funcTypeCxt);
  WasmEdge_ValType val_type_list_ret[ret_len];
  if (ret_len != WasmEdge_FunctionTypeGetReturns(funcTypeCxt, val_type_list_ret,
                                                 ret_len)) {
    /* TODO: Handle errors gracefully */
    return pybind11::make_tuple(NULL, NULL);
  };

  WasmEdge_Value Returns[ret_len];
  pysdk::result res(WasmEdge_ExecutorInvoke(context, st.get(), funcName, Params,
                                            param_len, Returns, ret_len));

  pybind11::list returns;

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
    // case WasmEdge_ValType_ExternRef:
    //   returns.append(pybind11::cast(WasmEdge_ValueGetExternRef(Returns[i])));
    //   break;
    default:
      break;
    }
  }

  return pybind11::make_tuple(res, returns);
}

pysdk::result pysdk::Executor::RegisterImport(pysdk::Store &store,
                                              pysdk::import_object &iobj) {
  return pysdk::result(
      WasmEdge_ExecutorRegisterImport(context, store.get(), iobj.get()));
}

pysdk::result pysdk::Executor::RegisterModule(pysdk::Store &store,
                                              pysdk::ASTModuleCxt &ast,
                                              std::string &str) {
  WasmEdge_String name = WasmEdge_StringCreateByCString(str.c_str());
  pysdk::result res(
      WasmEdge_ExecutorRegisterModule(context, store.get(), ast.get(), name));
  WasmEdge_StringDelete(name);
  return res;
}
/* --------------- Executor End -------------------------------- */
