#include "WasmEdge.hpp"

/* --------------- Ref -------------------------------- */

pysdk::Ref::Ref(pybind11::object obj) {
  if (pybind11::isinstance<WasmEdge_RefType>(obj)) {
    Val = WasmEdge_ValueGenNullRef(obj.cast<WasmEdge_RefType>());
  } else if (pybind11::isinstance<pysdk::Value>(obj)) {
    Val = obj.cast<pysdk::Value>().get();
    Ptr = WasmEdge_ValueGetExternRef(Val);
  } else {
    Ptr = (void *)obj.ptr();
    Val = WasmEdge_ValueGenExternRef(Ptr);
  }
}

pysdk::Ref::~Ref() {}

void *pysdk::Ref::get() { return Ptr; }

WasmEdge_Value pysdk::Ref::get_val() { return Val; }

uint32_t pysdk::Ref::get_function_index() {
  return WasmEdge_ValueGetFuncIdx(Val);
}

bool pysdk::Ref::is_null() { return WasmEdge_ValueIsNullRef(Val); }

WasmEdge_ValType pysdk::Ref::get_type() { return Val.Type; }

pybind11::object pysdk::Ref::get_py_obj() {
  pybind11::object obj;
  obj.ptr() = (PyObject *)Ptr;
  return obj;
}
/* --------------- Ref End -------------------------------- */
