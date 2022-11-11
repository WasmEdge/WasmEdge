#include "WasmEdge.hpp"

/* --------------- ASTModule -------------------------------- */

pysdk::ASTModuleCxt::ASTModuleCxt() { context = NULL; }

pysdk::ASTModuleCxt::ASTModuleCxt(const WasmEdge_ASTModuleContext *cxt)
    : base(cxt) {}

pysdk::ASTModuleCxt::~ASTModuleCxt() {
  if (_del)
    WasmEdge_ASTModuleDelete(context);
}

WasmEdge_ASTModuleContext **pysdk::ASTModuleCxt::get_addr() { return &context; }

pybind11::list pysdk::ASTModuleCxt::ListImports(uint32_t &len) {
  pybind11::list ret;
  const WasmEdge_ImportTypeContext *import_cxt_types[len];
  len = WasmEdge_ASTModuleListImports(context, import_cxt_types, len);
  for (size_t i = 0; i < len; i++) {
    ret.append((pysdk::ImportTypeContext(
        const_cast<WasmEdge_ImportTypeContext *>(import_cxt_types[i]))));
  }
  return ret;
}

pybind11::list pysdk::ASTModuleCxt::ListExports(uint32_t &len) {
  pybind11::list ret;
  const WasmEdge_ExportTypeContext *export_cxt_types[len];

  len = WasmEdge_ASTModuleListExports(context, export_cxt_types, len);
  for (size_t i = 0; i < len; i++) {
    ret.append(pysdk::ExportType(
        const_cast<WasmEdge_ExportTypeContext *>(export_cxt_types[i])));
  }
  return ret;
}

uint32_t pysdk::ASTModuleCxt::ListImportsLength() {
  return WasmEdge_ASTModuleListImportsLength(context);
}

uint32_t pysdk::ASTModuleCxt::ListExportsLength() {
  return WasmEdge_ASTModuleListExportsLength(context);
}
/* --------------- ASTModule End -------------------------------- */