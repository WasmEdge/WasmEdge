#include "WasmEdge.hpp"

/* --------------- Async -------------------------------- */
pysdk::Async::Async() {}

pysdk::Async::~Async() {}

pysdk::Async::Async(WasmEdge_Async *as) { async = as; }

WasmEdge_Async *pysdk::Async::get() { return async; }

pybind11::tuple pysdk::Async::Get(uint32_t &len) {
  WasmEdge_Value vals[len];
  pysdk::result res(WasmEdge_AsyncGet(async, vals, len));
  pybind11::list ret;
  for (size_t i = 0; i < len; i++) {
    ret.append(pysdk::Value(vals));
  }
  return pybind11::make_tuple(res, ret);
}

uint32_t pysdk::Async::GetReturnsLength() {
  return WasmEdge_AsyncGetReturnsLength(async);
}

void pysdk::Async::Wait() { WasmEdge_AsyncWait(async); }

bool pysdk::Async::WaitFor(uint64_t &msec) {
  return WasmEdge_AsyncWaitFor(async, msec);
}

void pysdk::Async::Cancel() { WasmEdge_AsyncCancel(async); }
/* --------------- Compiler End -------------------------------- */