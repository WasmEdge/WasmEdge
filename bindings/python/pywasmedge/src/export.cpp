#include "WasmEdge.hpp"

/* --------------- ExportType -------------------------------- */
pysdk::ExportType::ExportType() {}

pysdk::ExportType::ExportType(const WasmEdge_ExportTypeContext *cxt)
    : base(cxt) {}

pysdk::ExportType::~ExportType() {}

std::string pysdk::ExportType::GetExternalName() {
  WasmEdge_String name = WasmEdge_ExportTypeGetExternalName(context);
  char c_name[name.Length];
  WasmEdge_StringCopy(name, c_name, name.Length);
  return std::string(c_name);
}

WasmEdge_ExternalType pysdk::ExportType::GetExternalType() {
  return WasmEdge_ExportTypeGetExternalType(context);
}

pysdk::FunctionTypeContext
pysdk::ExportType::GetFunctionType(pysdk::ASTModuleCxt &ast) {
  return pysdk::FunctionTypeContext(const_cast<WasmEdge_FunctionTypeContext *>(
      WasmEdge_ExportTypeGetFunctionType(ast.get(), context)));
}
pysdk::GlobalTypeCxt
pysdk::ExportType::GetGlobalType(pysdk::ASTModuleCxt &ast) {
  return pysdk::GlobalTypeCxt(
      WasmEdge_ExportTypeGetGlobalType(ast.get(), context));
}

pysdk::MemoryTypeCxt
pysdk::ExportType::GetMemoryType(pysdk::ASTModuleCxt &ast) {
  return pysdk::MemoryTypeCxt(
      WasmEdge_ExportTypeGetMemoryType(ast.get(), context));
}
/* --------------- ExportType End -------------------------------- */