#include "WasmEdge.hpp"

/* --------------- StatisticsContext -------------------------------- */
pysdk::StatisticsContext::StatisticsContext(WasmEdge_StatisticsContext *cxt) {
  StatCxt = cxt;
  external = true;
}

pysdk::StatisticsContext::~StatisticsContext() {
  if (!external)
    WasmEdge_StatisticsDelete(StatCxt);
}
/* --------------- StatisticsContext End -------------------------------- */
