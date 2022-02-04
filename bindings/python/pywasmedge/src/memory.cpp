#include "WasmEdge.hpp"

/* --------------- MemoryTypeCxt -------------------------------- */
pysdk::MemoryTypeCxt::MemoryTypeCxt(WasmEdge_Limit &Lim) {
  MemTypeCxt = WasmEdge_MemoryTypeCreate(Lim);
}

pysdk::MemoryTypeCxt::~MemoryTypeCxt() {
  WasmEdge_MemoryTypeDelete(MemTypeCxt);
}

WasmEdge_MemoryTypeContext *pysdk::MemoryTypeCxt::get() { return MemTypeCxt; }
/* --------------- MemoryTypeCxt End -------------------------------- */
