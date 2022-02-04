#include "WasmEdge.hpp"

/* --------------- GlobalTypeCxt -------------------------------- */
pysdk::GlobalTypeCxt::GlobalTypeCxt(const WasmEdge_ValType &Type,
                                    const WasmEdge_Mutability &mut) {
  GlobTypeCxt = WasmEdge_GlobalTypeCreate(Type, mut);
}

pysdk::GlobalTypeCxt::~GlobalTypeCxt() {
  WasmEdge_GlobalTypeDelete(GlobTypeCxt);
}

WasmEdge_GlobalTypeContext *pysdk::GlobalTypeCxt::get() { return GlobTypeCxt; }
/* --------------- GlobalTypeCxt End -------------------------------- */
