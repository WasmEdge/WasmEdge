#include "WasmEdge.hpp"

/* --------------- Async -------------------------------- */
pysdk::Async::Async() {}

pysdk::Async::~Async() {
  if (_del) {
    WasmEdge_AsyncDelete(context);
  }
}

pysdk::Async::Async(const WasmEdge_Async *async) : base(async) {}

pybind11::tuple pysdk::Async::Get(uint32_t &len) {
  WasmEdge_Value vals[len];
  pysdk::result res(WasmEdge_AsyncGet(context, vals, len));
  pybind11::list ret;
  for (size_t i = 0; i < len; i++) {
    ret.append(pysdk::Value(vals));
  }
  return pybind11::make_tuple(res, ret);
}

uint32_t pysdk::Async::GetReturnsLength() {
  return WasmEdge_AsyncGetReturnsLength(context);
}

void pysdk::Async::Wait() { WasmEdge_AsyncWait(context); }

bool pysdk::Async::WaitFor(uint64_t &msec) {
  return WasmEdge_AsyncWaitFor(context, msec);
}

void pysdk::Async::Cancel() { WasmEdge_AsyncCancel(context); }
/* --------------- Compiler End -------------------------------- */