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

/* --------------- Global -------------------------------- */
pysdk::Global::Global() {}

pysdk::Global::Global(pysdk::GlobalTypeCxt &type, pysdk::Value &val) {
  Glob = WasmEdge_GlobalInstanceCreate(type.get(), val.get());
}

pysdk::Global::Global(WasmEdge_GlobalInstanceContext *cxt, bool del) {
  Glob = cxt;
  delete_cxt = del;
}

pysdk::Global::~Global() {
  if (delete_cxt)
    WasmEdge_GlobalInstanceDelete(Glob);
}

WasmEdge_GlobalInstanceContext *pysdk::Global::get() { return Glob; }

pysdk::GlobalTypeCxt pysdk::Global::GetGlobalType() {
  return pysdk::GlobalTypeCxt(
      const_cast<WasmEdge_GlobalTypeContext *>(
          WasmEdge_GlobalInstanceGetGlobalType(
              const_cast<const WasmEdge_GlobalInstanceContext *>(Glob))),
      false);
}

pysdk::Value pysdk::Global::GetValue() {
  return pysdk::Value(WasmEdge_GlobalInstanceGetValue(
      const_cast<const WasmEdge_GlobalInstanceContext *>(Glob)));
}

void pysdk::Global::SetValue(pysdk::Value &val) {
  WasmEdge_GlobalInstanceSetValue(Glob, val.get());
}
/* --------------- Global End -------------------------------- */