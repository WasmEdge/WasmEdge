#include "WasmEdge.hpp"

/* --------------- ASTModule -------------------------------- */

pysdk::ASTModuleCxt::ASTModuleCxt() { ASTCxt = NULL; }

pysdk::ASTModuleCxt::ASTModuleCxt(WasmEdge_ASTModuleContext *cxt) {
  ASTCxt = cxt;
  external = true;
}

pysdk::ASTModuleCxt::~ASTModuleCxt() {
  if (!external)
    WasmEdge_ASTModuleDelete(ASTCxt);
}

WasmEdge_ASTModuleContext *pysdk::ASTModuleCxt::get() { return ASTCxt; }
WasmEdge_ASTModuleContext **pysdk::ASTModuleCxt::get_addr() { return &ASTCxt; }

pybind11::list pysdk::ASTModuleCxt::listImports() {
  pybind11::list ret;
  auto len = WasmEdge_ASTModuleListImportsLength(ASTCxt);
  const WasmEdge_ImportTypeContext *import_cxt_types[len];
  len = WasmEdge_ASTModuleListImports(ASTCxt, import_cxt_types, len);
  for (size_t i = 0; i < len; i++) {
    // TODO: Implement in a useful way
  }

  return ret;
}

pybind11::list pysdk::ASTModuleCxt::listExports() {
  pybind11::list ret;
  auto len = WasmEdge_ASTModuleListExportsLength(ASTCxt);
  const WasmEdge_ExportTypeContext *export_cxt_types[len];
  len = WasmEdge_ASTModuleListExports(ASTCxt, export_cxt_types, len);
  for (size_t i = 0; i < len; i++) {
    // TODO: Implement in a useful way
  }

  return ret;
}

/* --------------- ASTModule End -------------------------------- */