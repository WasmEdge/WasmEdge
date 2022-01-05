#include "WasmEdge.hpp"

/* --------------- Validator -------------------------------- */

pysdk::Validator::Validator(pysdk::Configure &conf) {
  ValidCxt = WasmEdge_ValidatorCreate(conf.get());
}

pysdk::Validator::~Validator() { WasmEdge_ValidatorDelete(ValidCxt); }

pysdk::result pysdk::Validator::validate(pysdk::ASTModuleCxt &ast) {
  return pysdk::result(WasmEdge_ValidatorValidate(ValidCxt, ast.get()));
}

/* --------------- Validator End -------------------------------- */
