#include "WasmEdge.hpp"

/* --------------- Statistics -------------------------------- */
pysdk::Statistics::Statistics() { context = WasmEdge_StatisticsCreate(); }

pysdk::Statistics::Statistics(const WasmEdge_StatisticsContext *cxt)
    : base(cxt) {}

pysdk::Statistics::~Statistics() {
  if (_del)
    WasmEdge_StatisticsDelete(context);
}

uint64_t pysdk::Statistics::GetInstrCount() {
  return WasmEdge_StatisticsGetInstrCount(context);
}

double pysdk::Statistics::GetInstrPerSecond() {
  return WasmEdge_StatisticsGetInstrPerSecond(context);
}

uint64_t pysdk::Statistics::GetTotalCost() {
  return WasmEdge_StatisticsGetTotalCost(context);
}

void pysdk::Statistics::SetCostLimit(uint64_t &lim) {
  WasmEdge_StatisticsSetCostLimit(context, lim);
}

void pysdk::Statistics::SetCostTable(pybind11::tuple tup) {
  auto const len = pybind11::len(tup);
  uint64_t cost_arr[len];
  for (size_t i = 0; i < len; i++) {
    cost_arr[i] = tup[i].cast<uint64_t>();
  }
  WasmEdge_StatisticsSetCostTable(context, cost_arr, len);
}
/* --------------- Statistics End -------------------------------- */
