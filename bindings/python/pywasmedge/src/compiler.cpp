#include "WasmEdge.hpp"

/* --------------- Compiler -------------------------------- */
pysdk::Compiler::Compiler(pysdk::Configure &cfg) {
  context = WasmEdge_CompilerCreate(cfg.get());
}

pysdk::Compiler::~Compiler() {
  if (_del)
    WasmEdge_CompilerDelete(context);
}

pysdk::result pysdk::Compiler::Compile(std::string &in, std::string &out) {
  return pysdk::result(
      WasmEdge_CompilerCompile(context, in.c_str(), out.c_str()));
}
/* --------------- Compiler End -------------------------------- */