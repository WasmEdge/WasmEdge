#include "WasmEdge.hpp"

/* --------------- Loader -------------------------------- */

pysdk::Loader::Loader(pysdk::Configure &cfg) {
  context = WasmEdge_LoaderCreate(cfg.get());
}

pysdk::Loader::~Loader() {
  if (_del)
    WasmEdge_LoaderDelete(context);
}

pysdk::result pysdk::Loader::parse(pysdk::ASTModuleCxt &ast,
                                   std::string &path) {
  return pysdk::result(
      WasmEdge_LoaderParseFromFile(context, ast.get_addr(), path.c_str()));
}

pysdk::result pysdk::Loader::parse(pysdk::ASTModuleCxt &ast,
                                   pybind11::tuple buf_) {
  auto const len = pybind11::len(buf_);
  uint8_t buf[len];
  for (size_t i = 0; i < len; i++) {
    buf[i] = buf_.cast<uint8_t>();
  }
  return pysdk::result(
      WasmEdge_LoaderParseFromBuffer(context, ast.get_addr(), buf, len));
}
/* --------------- Loader End -------------------------------- */
