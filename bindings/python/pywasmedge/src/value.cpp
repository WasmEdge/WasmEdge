#include "WasmEdge.hpp"

/* --------------- Value -------------------------------- */
pysdk::Value::Value(pybind11::object obj_, WasmEdge_ValType &type) {
  set_value(obj_, type);
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
    // TODO: Handle Ref here itself
  default:
    throw std::runtime_error("Data type not supported currently");
    break;
  }
}

WasmEdge_Value pysdk::Value::get() { return Val; }

/* --------------- Value End -------------------------------- */
