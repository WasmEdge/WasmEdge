#include "WasmEdge.hpp"

/* --------------- ExportType -------------------------------- */
pysdk::ExportType::ExportType() {}

pysdk::ExportType::ExportType(WasmEdge_ExportTypeContext *cxt) {
  ExpoCxt = cxt;
}

pysdk::ExportType::~ExportType() {}

WasmEdge_ExportTypeContext *pysdk::ExportType::get() { return ExpoCxt; }

std::string pysdk::ExportType::GetExternalName() {
  WasmEdge_String name = WasmEdge_ExportTypeGetExternalName(ExpoCxt);
  char c_name[name.Length];
  WasmEdge_StringCopy(name, c_name, name.Length);
  return std::string(c_name);
}

WasmEdge_ExternalType pysdk::ExportType::GetExternalType() {
  return WasmEdge_ExportTypeGetExternalType(ExpoCxt);
}

pysdk::FunctionTypeContext
pysdk::ExportType::GetFunctionType(pysdk::ASTModuleCxt &ast) {
  return pysdk::FunctionTypeContext(const_cast<WasmEdge_FunctionTypeContext *>(
      WasmEdge_ExportTypeGetFunctionType(
          const_cast<const WasmEdge_ASTModuleContext *>(ast.get()), ExpoCxt)));
}
pysdk::GlobalTypeCxt
pysdk::ExportType::GetGlobalType(pysdk::ASTModuleCxt &ast) {
  return pysdk::GlobalTypeCxt(
      const_cast<WasmEdge_GlobalTypeContext *>(WasmEdge_ExportTypeGetGlobalType(
          const_cast<const WasmEdge_ASTModuleContext *>(ast.get()), ExpoCxt)),
      false);
}

pysdk::MemoryTypeCxt
pysdk::ExportType::GetMemoryType(pysdk::ASTModuleCxt &ast) {
  return pysdk::MemoryTypeCxt(
      const_cast<WasmEdge_MemoryTypeContext *>(WasmEdge_ExportTypeGetMemoryType(
          const_cast<const WasmEdge_ASTModuleContext *>(ast.get()), ExpoCxt)),
      false);
}
/* --------------- ExportType End -------------------------------- */