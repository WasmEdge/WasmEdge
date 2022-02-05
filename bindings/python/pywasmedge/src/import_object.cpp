#include "WasmEdge.hpp"

/* --------------- import_object End ----------------------------------------*/

pysdk::import_object::import_object(std::string &name) {
  ModCxt =
      WasmEdge_ImportObjectCreate(WasmEdge_StringCreateByCString(name.c_str()));
}

void pysdk::import_object::add(pysdk::function &func, std::string &name) {
  WasmEdge_String function_name = WasmEdge_StringCreateByCString(name.c_str());
  WasmEdge_ImportObjectAddFunction(ModCxt, function_name, func.get());
  WasmEdge_StringDelete(function_name);
}

pysdk::import_object::~import_object() {}

WasmEdge_ImportObjectContext *pysdk::import_object::get() { return ModCxt; }

/* --------------- import_object End ----------------------------------------*/
