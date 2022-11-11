#include "WasmEdge.hpp"

/* --------------- GlobalTypeCxt -------------------------------- */
pysdk::GlobalTypeCxt::GlobalTypeCxt(const WasmEdge_ValType &Type,
                                    const WasmEdge_Mutability &mut) {
  context = WasmEdge_GlobalTypeCreate(Type, mut);
}

pysdk::GlobalTypeCxt::GlobalTypeCxt(const WasmEdge_GlobalTypeContext *cxt)
    : base(cxt) {}

pysdk::GlobalTypeCxt::~GlobalTypeCxt() {
  if (_del)
    WasmEdge_GlobalTypeDelete(context);
}

WasmEdge_Mutability pysdk::GlobalTypeCxt::GetMutability() {
  return WasmEdge_GlobalTypeGetMutability(context);
}

WasmEdge_ValType pysdk::GlobalTypeCxt::GetValType() {
  return WasmEdge_GlobalTypeGetValType(context);
}
/* --------------- GlobalTypeCxt End -------------------------------- */

/* --------------- Global -------------------------------- */
pysdk::Global::Global(pysdk::GlobalTypeCxt &type, pysdk::Value &val) {
  context = WasmEdge_GlobalInstanceCreate(type.get(), val.get());
}

pysdk::Global::Global(const WasmEdge_GlobalInstanceContext *cxt) : base(cxt) {}

pysdk::Global::~Global() {
  if (_del)
    WasmEdge_GlobalInstanceDelete(context);
}

pysdk::GlobalTypeCxt pysdk::Global::GetGlobalType() {
  return pysdk::GlobalTypeCxt(WasmEdge_GlobalInstanceGetGlobalType(get()));
}

pysdk::Value pysdk::Global::GetValue() {
  return pysdk::Value(WasmEdge_GlobalInstanceGetValue(get()));
}

void pysdk::Global::SetValue(pysdk::Value &val) {
  WasmEdge_GlobalInstanceSetValue(context, val.get());
}
/* --------------- Global End -------------------------------- */