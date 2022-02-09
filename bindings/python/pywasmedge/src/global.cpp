#include "WasmEdge.hpp"

/* --------------- GlobalTypeCxt -------------------------------- */
pysdk::GlobalTypeCxt::GlobalTypeCxt(const WasmEdge_ValType &Type,
                                    const WasmEdge_Mutability &mut) {
  GlobTypeCxt = WasmEdge_GlobalTypeCreate(Type, mut);
}

pysdk::GlobalTypeCxt::GlobalTypeCxt(WasmEdge_GlobalTypeContext *cxt, bool del) {
  GlobTypeCxt = cxt;
  delete_cxt = del;
}

pysdk::GlobalTypeCxt::~GlobalTypeCxt() {
  if (delete_cxt)
    WasmEdge_GlobalTypeDelete(GlobTypeCxt);
}

WasmEdge_GlobalTypeContext *pysdk::GlobalTypeCxt::get() { return GlobTypeCxt; }
/* --------------- GlobalTypeCxt End -------------------------------- */
