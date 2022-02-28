#include "WasmEdge.hpp"

/* --------------- Compiler -------------------------------- */
pysdk::Compiler::Compiler(pysdk::Configure &cfg) {
  cxt = WasmEdge_CompilerCreate(cfg.get());
}

pysdk::Compiler::~Compiler() { WasmEdge_CompilerDelete(cxt); }

pysdk::result pysdk::Compiler::Compile(std::string &in, std::string &out) {
  return pysdk::result(WasmEdge_CompilerCompile(cxt, in.c_str(), out.c_str()));
}
/* --------------- Compiler End -------------------------------- */