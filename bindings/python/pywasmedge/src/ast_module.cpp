#include "WasmEdge.hpp"

/* --------------- ASTModule -------------------------------- */

pysdk::ASTModuleCxt::ASTModuleCxt() { ASTCxt = NULL; }

pysdk::ASTModuleCxt::ASTModuleCxt(WasmEdge_ASTModuleContext *cxt) {
  ASTCxt = cxt;
  external = true;
}

pysdk::ASTModuleCxt::~ASTModuleCxt() {
  if (!external && ASTCxt != NULL)
    WasmEdge_ASTModuleDelete(ASTCxt);
}

WasmEdge_ASTModuleContext *pysdk::ASTModuleCxt::get() { return ASTCxt; }

WasmEdge_ASTModuleContext **pysdk::ASTModuleCxt::get_addr() { return &ASTCxt; }

pybind11::list pysdk::ASTModuleCxt::ListImports(uint32_t &len) {
  pybind11::list ret;
  const WasmEdge_ImportTypeContext *import_cxt_types[len];
  len = WasmEdge_ASTModuleListImports(ASTCxt, import_cxt_types, len);
  for (size_t i = 0; i < len; i++) {
    ret.append((pysdk::ImportTypeContext(
        const_cast<WasmEdge_ImportTypeContext *>(import_cxt_types[i]))));
  }
  return ret;
}

pybind11::list pysdk::ASTModuleCxt::ListExports(uint32_t &len) {
  pybind11::list ret;
  const WasmEdge_ExportTypeContext *export_cxt_types[len];

  len = WasmEdge_ASTModuleListExports(ASTCxt, export_cxt_types, len);
  for (size_t i = 0; i < len; i++) {
    ret.append(pysdk::ExportType(
        const_cast<WasmEdge_ExportTypeContext *>(export_cxt_types[i])));
  }
  return ret;
}

uint32_t pysdk::ASTModuleCxt::ListImportsLength() {
  return WasmEdge_ASTModuleListImportsLength(ASTCxt);
}

uint32_t pysdk::ASTModuleCxt::ListExportsLength() {
  return WasmEdge_ASTModuleListExportsLength(ASTCxt);
}
/* --------------- ASTModule End -------------------------------- */