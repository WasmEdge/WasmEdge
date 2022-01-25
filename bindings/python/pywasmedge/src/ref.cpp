#include "WasmEdge.hpp"

/* --------------- Ref -------------------------------- */

pysdk::Ref::Ref(pybind11::object type, pybind11::object obj) {
  switch (type.cast<WasmEdge_RefType>()) {
  case WasmEdge_RefType_ExternRef:
    if (pybind11::isinstance<pysdk::Value>(obj)) {
      Val = obj.cast<pysdk::Value>().get();
      Ptr = WasmEdge_ValueGetExternRef(Val);
    } else {
      if (!obj.is_none()) {
        Ptr = (void *)obj.ptr();
        Val = WasmEdge_ValueGenExternRef(Ptr);
      } else {
        Val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_ExternRef);
        Ptr = WasmEdge_ValueGetExternRef(Val);
      }
    }
    break;
  case WasmEdge_RefType_FuncRef:
    if (!obj.is_none()) {
      Val = WasmEdge_ValueGenFuncRef(obj.cast<uint32_t>());
    } else {
      Val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_FuncRef);
    }
    Ptr = (void *)obj.ptr();
    break;
  default:
    throw std::runtime_error(
        "Ref Error:" + type.get_type().cast<std::string>() +
        " is unsupported WasmEdge_RefType");
    break;
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
