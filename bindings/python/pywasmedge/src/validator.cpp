#include "WasmEdge.hpp"

/* --------------- Validator -------------------------------- */

pysdk::Validator::Validator(pysdk::Configure &conf) {
  context = WasmEdge_ValidatorCreate(conf.get());
}

pysdk::Validator::~Validator() {
  if (_del)
    WasmEdge_ValidatorDelete(context);
}

pysdk::result pysdk::Validator::validate(pysdk::ASTModuleCxt &ast) {
  return pysdk::result(WasmEdge_ValidatorValidate(context, ast.get()));
}
/* --------------- Validator End -------------------------------- */
