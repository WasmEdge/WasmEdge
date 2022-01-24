#include "WasmEdge.hpp"

/* --------------- Value -------------------------------- */
pysdk::Value::Value(pybind11::object obj_) { set_value(obj_); }

pysdk::Value::~Value() {}

pybind11::object pysdk::Value::get_value() { return obj; }

WasmEdge_ValType pysdk::Value::get_type() { return Val.Type; }

void pysdk::Value::set_value(pybind11::object obj_) {
  obj = obj_;
  if (pybind11::isinstance<pybind11::int_>(obj)) {
    Val = WasmEdge_ValueGenI32(obj.cast<int32_t>());
  } else if (pybind11::isinstance<int64_t>(obj)) {
    Val = WasmEdge_ValueGenI64(obj.cast<int64_t>());
  } else if (pybind11::isinstance<pybind11::float_>(obj)) {
    Val = WasmEdge_ValueGenF32(obj.cast<float>());
  } else if (pybind11::isinstance<double>(obj)) {
    Val = WasmEdge_ValueGenF64(obj.cast<double>());
  } else {
    throw std::runtime_error("Value class in Python does not support " +
                             obj.get_type().cast<std::string>());
  }
}

WasmEdge_Value pysdk::Value::get() { return Val; }

/* --------------- Value End -------------------------------- */
