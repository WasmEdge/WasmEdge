#include "WasmEdge.hpp"

/* --------------- import_object End ----------------------------------------*/

pysdk::import_object::import_object(std::string &name) {
  ModCxt =
      WasmEdge_ImportObjectCreate(WasmEdge_StringCreateByCString(name.c_str()));
}

pysdk::import_object::import_object(WasmEdge_ImportObjectContext *cxt) {
  ModCxt = cxt;
}

void pysdk::import_object::add(pysdk::Function &func, std::string &name) {
  WasmEdge_String function_name = WasmEdge_StringCreateByCString(name.c_str());
  WasmEdge_ImportObjectAddFunction(ModCxt, function_name, func.get());
  WasmEdge_StringDelete(function_name);
}

pysdk::import_object::~import_object() {}

WasmEdge_ImportObjectContext *pysdk::import_object::get() { return ModCxt; }

/* --------------- import_object End ----------------------------------------*/

/* --------------- ImportTypeContext ----------------------------------------*/
pysdk::ImportTypeContext::ImportTypeContext() {}

pysdk::ImportTypeContext::ImportTypeContext(WasmEdge_ImportTypeContext *cxt) {
  Cxt = cxt;
  // WasmEdge_ImportTypeGetMemoryType();
  // WasmEdge_ImportTypeGetModuleName();
  // WasmEdge_ImportTypeGetTableType();
}

pysdk::ImportTypeContext::~ImportTypeContext() {}

std::string pysdk::ImportTypeContext::get_external_name() {
  const WasmEdge_String name = WasmEdge_ImportTypeGetExternalName(
      const_cast<const WasmEdge_ImportTypeContext *>(Cxt));
  char buf[name.Length];
  WasmEdge_StringCopy(name, buf, name.Length);
  return std::string(buf);
}

WasmEdge_ExternalType pysdk::ImportTypeContext::get_external_type() {
  return WasmEdge_ImportTypeGetExternalType(
      const_cast<const WasmEdge_ImportTypeContext *>(Cxt));
}

pysdk::FunctionTypeContext
pysdk::ImportTypeContext::get_function_type_cxt(pysdk::ASTModuleCxt &ast_cxt) {
  return pysdk::FunctionTypeContext(const_cast<WasmEdge_FunctionTypeContext *>(
      WasmEdge_ImportTypeGetFunctionType(
          const_cast<const WasmEdge_ASTModuleContext *>(ast_cxt.get()), Cxt)));
}

pysdk::GlobalTypeCxt
pysdk::ImportTypeContext::get_global_type_cxt(pysdk::ASTModuleCxt &ast_cxt) {
  return pysdk::GlobalTypeCxt(
      const_cast<WasmEdge_GlobalTypeContext *>(WasmEdge_ImportTypeGetGlobalType(
          const_cast<const WasmEdge_ASTModuleContext *>(ast_cxt.get()),
          const_cast<const WasmEdge_ImportTypeContext *>(Cxt))),
      false);
}

WasmEdge_ImportTypeContext *pysdk::ImportTypeContext::get() { return Cxt; }
/* --------------- ImportTypeContext End -----------------------------------*/
