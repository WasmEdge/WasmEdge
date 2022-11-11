#include "WasmEdge.hpp"

/* --------------- Value -------------------------------- */
pysdk::Value::Value(pybind11::object obj_, WasmEdge_ValType &type) {
  set_value(obj_, type);
}

pysdk::Value::Value(WasmEdge_Value *val) {
  Val = *val;
  switch (val->Type) {
  case WasmEdge_ValType_I32:
    obj = pybind11::int_(WasmEdge_ValueGetI32(Val));
    break;
  case WasmEdge_ValType_I64:
    obj = pybind11::int_(WasmEdge_ValueGetI64(Val));
    break;
  case WasmEdge_ValType_F32:
    obj = pybind11::float_(WasmEdge_ValueGetF32(Val));
    break;
  case WasmEdge_ValType_F64:
    obj = pybind11::float_(WasmEdge_ValueGetF64(Val));
    break;
  case WasmEdge_ValType_ExternRef:
    obj.ptr() = (PyObject *)WasmEdge_ValueGetExternRef(Val);
    break;
  case WasmEdge_ValType_FuncRef:
    obj = pybind11::int_(WasmEdge_ValueGetFuncIdx(Val));
    break;
  default:
    throw std::runtime_error(
        __FILE__ ": L" + std::to_string(__LINE__) +
        " :Unknown Value type:" + std::to_string(val->Type));
    break;
  }
}

pysdk::Value::Value(const WasmEdge_Value &val) {
  Val = val;
  switch (val.Type) {
  case WasmEdge_ValType_I32:
    obj = pybind11::int_(WasmEdge_ValueGetI32(Val));
    break;
  case WasmEdge_ValType_I64:
    obj = pybind11::int_(WasmEdge_ValueGetI64(Val));
    break;
  case WasmEdge_ValType_F32:
    obj = pybind11::float_(WasmEdge_ValueGetF32(Val));
    break;
  case WasmEdge_ValType_F64:
    obj = pybind11::float_(WasmEdge_ValueGetF64(Val));
    break;
  case WasmEdge_ValType_ExternRef:
    obj.ptr() = (PyObject *)WasmEdge_ValueGetExternRef(Val);
    break;
  case WasmEdge_ValType_FuncRef:
    obj = pybind11::int_(WasmEdge_ValueGetFuncIdx(Val));
    break;
  default:
    throw std::runtime_error(
        __FILE__ ": L" + std::to_string(__LINE__) +
        " :Unknown Value type:" + std::to_string(val.Type));
    break;
  }
}

pysdk::Value::~Value() {}

pybind11::object pysdk::Value::get_value() { return obj; }

WasmEdge_ValType pysdk::Value::get_type() { return Val.Type; }

void pysdk::Value::set_value(pybind11::object obj_, WasmEdge_ValType &type) {
  obj = obj_;
  switch (type) {
  case WasmEdge_ValType_I32:
    Val = WasmEdge_ValueGenI32(obj.cast<int32_t>());
    break;
  case WasmEdge_ValType_I64:
    Val = WasmEdge_ValueGenI64(obj.cast<int64_t>());
    break;
  case WasmEdge_ValType_F32:
    Val = WasmEdge_ValueGenF32(obj.cast<float>());
    break;
  case WasmEdge_ValType_F64:
    Val = WasmEdge_ValueGenF64(obj.cast<double>());
    break;
  case WasmEdge_ValType_ExternRef:
    Val = WasmEdge_ValueGenExternRef((void *)obj.ptr());
    break;
  case WasmEdge_ValType_FuncRef:
    Val = WasmEdge_ValueGenFuncRef(obj.cast<uint32_t>());
    break;
  default:
    throw std::runtime_error(__FILE__ ": L" + std::to_string(__LINE__) +
                             " :Unknown Value type:" + std::to_string(type));
    break;
  }
}

WasmEdge_Value pysdk::Value::get() { return Val; }

/* --------------- Value End -------------------------------- */
