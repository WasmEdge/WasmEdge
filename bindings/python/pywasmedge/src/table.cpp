#include "WasmEdge.hpp"

/* --------------- TableTypeCxt -------------------------------- */
pysdk::TableTypeCxt::TableTypeCxt(WasmEdge_RefType &type, WasmEdge_Limit &Lim) {
  context = WasmEdge_TableTypeCreate(type, Lim);
}

pysdk::TableTypeCxt::TableTypeCxt(const WasmEdge_TableTypeContext *cxt)
    : base(cxt) {}

pysdk::TableTypeCxt::~TableTypeCxt() {
  if (_del)
    WasmEdge_TableTypeDelete(context);
}

WasmEdge_Limit pysdk::TableTypeCxt::GetLimit() {
  return WasmEdge_TableTypeGetLimit(context);
}

WasmEdge_RefType pysdk::TableTypeCxt::GetRefType() {
  return WasmEdge_TableTypeGetRefType(context);
}
/* --------------- TableTypeCxt End -------------------------------- */

/* --------------- Table End -------------------------------- */
pysdk::Table::Table(pysdk::TableTypeCxt &tab_cxt) {
  context = WasmEdge_TableInstanceCreate(tab_cxt.get());
}

pysdk::Table::Table(const WasmEdge_TableInstanceContext *tab_cxt)
    : base(tab_cxt) {}

pysdk::Table::~Table() {
  if (_del)
    WasmEdge_TableInstanceDelete(context);
}

pysdk::TableTypeCxt pysdk::Table::get_type() {
  return pysdk::TableTypeCxt(WasmEdge_TableInstanceGetTableType(context));
}

uint32_t pysdk::Table::get_size() {
  return WasmEdge_TableInstanceGetSize(get());
}

pysdk::result pysdk::Table::grow_size(const uint32_t &size) {
  return pysdk::result(WasmEdge_TableInstanceGrow(context, size));
}

pysdk::result pysdk::Table::set_data(pysdk::Value &value,
                                     const uint32_t &offset) {
  return pysdk::result(
      WasmEdge_TableInstanceSetData(context, value.get(), offset));
}

pybind11::tuple pysdk::Table::get_data(const uint32_t &offset) {
  WasmEdge_Value val_;
  pysdk::result res(WasmEdge_TableInstanceGetData(context, &val_, offset));
  pysdk::Value val(&val_);
  return pybind11::make_tuple(res, val);
}
/* --------------- Table End -------------------------------- */
