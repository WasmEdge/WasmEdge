#include "WasmEdge.hpp"

/* --------------- TableTypeCxt -------------------------------- */
pysdk::TableTypeCxt::TableTypeCxt(WasmEdge_RefType &type, WasmEdge_Limit &Lim) {
  TabTypeCxt = WasmEdge_TableTypeCreate(type, Lim);
}

pysdk::TableTypeCxt::TableTypeCxt(const WasmEdge_TableTypeContext *cxt) {
  TabTypeCxt = const_cast<WasmEdge_TableTypeContext *>(cxt);
  external = true;
}

pysdk::TableTypeCxt::~TableTypeCxt() {
  if (!external)
    WasmEdge_TableTypeDelete(TabTypeCxt);
}

WasmEdge_TableTypeContext *pysdk::TableTypeCxt::get() { return TabTypeCxt; }

WasmEdge_Limit pysdk::TableTypeCxt::GetLimit() {
  return WasmEdge_TableTypeGetLimit(TabTypeCxt);
}

WasmEdge_RefType pysdk::TableTypeCxt::GetRefType() {
  return WasmEdge_TableTypeGetRefType(TabTypeCxt);
}
/* --------------- TableTypeCxt End -------------------------------- */

/* --------------- Table End -------------------------------- */
pysdk::Table::Table(pysdk::TableTypeCxt &tab_cxt) {
  HostTable = WasmEdge_TableInstanceCreate(tab_cxt.get());
}

pysdk::Table::Table(WasmEdge_TableInstanceContext *tab_cxt, bool del) {
  delete_cxt = del;
  HostTable = tab_cxt;
}

pysdk::Table::~Table() {
  if (delete_cxt)
    WasmEdge_TableInstanceDelete(HostTable);
}

WasmEdge_TableInstanceContext *pysdk::Table::get() { return HostTable; }

pysdk::TableTypeCxt pysdk::Table::get_type() {
  return pysdk::TableTypeCxt(WasmEdge_TableInstanceGetTableType(HostTable));
}

uint32_t pysdk::Table::get_size() {
  return WasmEdge_TableInstanceGetSize(
      const_cast<const WasmEdge_TableInstanceContext *>(HostTable));
}

pysdk::result pysdk::Table::grow_size(const uint32_t &size) {
  return pysdk::result(WasmEdge_TableInstanceGrow(HostTable, size));
}

pysdk::result pysdk::Table::set_data(pysdk::Value &value,
                                     const uint32_t &offset) {
  return pysdk::result(
      WasmEdge_TableInstanceSetData(HostTable, value.get(), offset));
}

pybind11::tuple pysdk::Table::get_data(const uint32_t &offset) {
  WasmEdge_Value val_;
  pysdk::result res(WasmEdge_TableInstanceGetData(HostTable, &val_, offset));
  pysdk::Value val(&val_);
  return pybind11::make_tuple(res, val);
}
/* --------------- Table End -------------------------------- */
