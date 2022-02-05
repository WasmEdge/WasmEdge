#include "WasmEdge.hpp"

/* --------------- FunctionTypeContext -------------------------------*/
pysdk::FunctionTypeConext::FunctionTypeConext(pybind11::list param_list,
                                              pybind11::list return_list) {
  auto const param_len = pybind11::len(param_list);
  auto const ret_len = pybind11::len(return_list);
  WasmEdge_ValType param_types[param_len];
  for (size_t i = 0; i < param_len; i++) {
    param_types[i] = param_list[i].cast<WasmEdge_ValType>();
  }
  WasmEdge_ValType ret_types[ret_len];
  for (size_t i = 0; i < ret_len; i++) {
    ret_types[i] = return_list[i].cast<WasmEdge_ValType>();
  }

  HostFType = WasmEdge_FunctionTypeCreate(
      const_cast<const WasmEdge_ValType *>(param_types), param_len,
      const_cast<const WasmEdge_ValType *>(ret_types), ret_len);

  if (HostFType == NULL) {
    throw std::runtime_error("Cannor Create Function Type");
  }
}

pysdk::FunctionTypeConext::~FunctionTypeConext() {
  WasmEdge_FunctionTypeDelete(HostFType);
}

WasmEdge_FunctionTypeContext *pysdk::FunctionTypeConext::get() {

  return HostFType;
}

uint32_t pysdk::FunctionTypeConext::get_param_len() {

  return WasmEdge_FunctionTypeGetParametersLength(
      const_cast<const WasmEdge_FunctionTypeContext *>(HostFType));
}

pybind11::list pysdk::FunctionTypeConext::get_param_types(const uint32_t &len) {
  pybind11::list ret;
  WasmEdge_ValType rets[len];
  auto const len_api = WasmEdge_FunctionTypeGetParameters(
      const_cast<const WasmEdge_FunctionTypeContext *>(HostFType), rets, len);
  for (size_t i = 0; i < len_api; i++) {
    ret.append(rets[i]);
  }
  return ret;
}

uint32_t pysdk::FunctionTypeConext::get_ret_len() {

  return WasmEdge_FunctionTypeGetReturnsLength(
      const_cast<const WasmEdge_FunctionTypeContext *>(HostFType));
}

pybind11::list pysdk::FunctionTypeConext::get_ret_types(const uint32_t &len) {
  pybind11::list ret;
  WasmEdge_ValType rets[len];
  auto const len_api = WasmEdge_FunctionTypeGetReturns(
      const_cast<const WasmEdge_FunctionTypeContext *>(HostFType), rets, len);
  for (size_t i = 0; i < len_api; i++) {
    ret.append(rets[i]);
  }
  return ret;
}
/* --------------- FunctionTypeContext End  ---------------------------------*/

/* --------------- Function ----------------------------------------*/

pysdk::function::function(pybind11::function func_)
    : func(func_), hfunc_util(new pysdk::function_utility) {
  pybind11::dict annotations = func.attr("__annotations__");
  auto total = pybind11::len(annotations);
  ret_len = pybind11::len(pybind11::make_tuple(annotations["return"]));
  param_len = total - ret_len;

  size_t i = 0;

  pybind11::int_ temp_int;
  pybind11::float_ temp_float;

  param_types = new WasmEdge_ValType[param_len];
  return_types = new WasmEdge_ValType[ret_len];

  for (auto ret : pybind11::make_tuple(annotations["return"])) {
    auto type_str = ret.cast<pybind11::type>();
    if (type_str.is(temp_int.get_type())) {
      return_types[i] = WasmEdge_ValType_I32;
    } else if (type_str.is(temp_float.get_type())) {
      return_types[i] = WasmEdge_ValType_F32;
    } else {
      // TODO: Handle Errors
    }
    i++;
  }

  i = 0;

  for (auto e : annotations) {
    if (e.first.cast<std::string>().compare("return") == 0) {
      continue;
    }
    auto type_str = e.second.cast<pybind11::type>();
    if (type_str.is(temp_int.get_type())) {
      param_types[i] = WasmEdge_ValType_I32;
    } else if (type_str.is(temp_float.get_type())) {
      param_types[i] = WasmEdge_ValType_F32;
    } else {
      // TODO: Handle Errors
    }
    i++;
  }

  HostFType = WasmEdge_FunctionTypeCreate(param_types, param_len, return_types,
                                          ret_len);

  hfunc_util->func = func;
  hfunc_util->param_len = param_len;

  HostFuncCxt = WasmEdge_FunctionInstanceCreate(HostFType, host_function,
                                                (void *)hfunc_util, 0);
}

WasmEdge_Result pysdk::host_function(void *Data,
                                     WasmEdge_MemoryInstanceContext *MemCxt,
                                     const WasmEdge_Value *In,
                                     WasmEdge_Value *Out) {
  pybind11::list params;

  auto casted_data = (struct function_utility *)Data;

  auto const &func = casted_data->func;

  size_t param_len = casted_data->param_len;

  for (size_t i = 0; i < param_len; i++) {
    switch (In[i].Type) {
    case WasmEdge_ValType_I32:
      params.append(WasmEdge_ValueGetI32(In[i]));
      break;
    case WasmEdge_ValType_F32:
      params.append(WasmEdge_ValueGetF32(In[i]));
      break;
    }
  }
  auto params_tup = params.cast<pybind11::tuple>();

  auto const ret = func(*params_tup);
  pybind11::tuple returns;
  if (!pybind11::isinstance<pybind11::tuple>(ret)) {
    returns = pybind11::make_tuple(ret);
  } else {
    returns = ret;
  }

  for (size_t i = 0; i < returns.size(); i++) {
    if (pybind11::isinstance<pybind11::int_>(returns[i])) {
      Out[i] = WasmEdge_ValueGenI32(returns[i].cast<int>());
    } else if (pybind11::isinstance<pybind11::float_>(returns[i])) {
      Out[i] = WasmEdge_ValueGenI32(returns[i].cast<float>());
    }
  }

  return WasmEdge_Result_Success;
};

WasmEdge_FunctionInstanceContext *pysdk::function::get() { return HostFuncCxt; }

pysdk::function::~function() {
  WasmEdge_FunctionInstanceDelete(HostFuncCxt);
  WasmEdge_FunctionTypeDelete(HostFType);
  delete[] param_types, return_types;
  delete hfunc_util;
}

/* --------------- Function End ----------------------------------------*/