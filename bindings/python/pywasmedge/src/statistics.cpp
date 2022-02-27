#include "WasmEdge.hpp"

/* --------------- Statistics -------------------------------- */
pysdk::Statistics::Statistics() {
  StatCxt = WasmEdge_StatisticsCreate();
  delete_stat = true;
}

pysdk::Statistics::Statistics(WasmEdge_StatisticsContext *cxt, bool del) {
  StatCxt = cxt;
  delete_stat = del;
}

pysdk::Statistics::~Statistics() {
  if (delete_stat)
    WasmEdge_StatisticsDelete(StatCxt);
}

uint64_t pysdk::Statistics::GetInstrCount() {
  return WasmEdge_StatisticsGetInstrCount(StatCxt);
}

double pysdk::Statistics::GetInstrPerSecond() {
  return WasmEdge_StatisticsGetInstrPerSecond(StatCxt);
}

uint64_t pysdk::Statistics::GetTotalCost() {
  return WasmEdge_StatisticsGetTotalCost(StatCxt);
}

void pysdk::Statistics::SetCostLimit(uint64_t &lim) {
  WasmEdge_StatisticsSetCostLimit(StatCxt, lim);
}

void pysdk::Statistics::SetCostTable(pybind11::tuple tup) {
  auto const len = pybind11::len(tup);
  uint64_t cost_arr[len];
  for (size_t i = 0; i < len; i++) {
    cost_arr[i] = tup[i].cast<uint64_t>();
  }
  WasmEdge_StatisticsSetCostTable(StatCxt, cost_arr, len);
}
/* --------------- Statistics End -------------------------------- */
