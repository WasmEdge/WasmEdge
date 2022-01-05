#include "WasmEdge.hpp"

/* --------------- Loader -------------------------------- */

pysdk::Loader::Loader(pysdk::Configure &cfg) {
  LoadCxt = WasmEdge_LoaderCreate(cfg.get());
}

pysdk::Loader::~Loader() { WasmEdge_LoaderDelete(LoadCxt); }

WasmEdge_LoaderContext *pysdk::Loader::get() { return LoadCxt; }

pysdk::result pysdk::Loader::parse(pysdk::ASTModuleCxt &ast,
                                   std::string &path) {
  return pysdk::result(
      WasmEdge_LoaderParseFromFile(LoadCxt, ast.get_addr(), path.c_str()));
}

pysdk::result pysdk::Loader::parse(pysdk::ASTModuleCxt &ast,
                                   pybind11::tuple buf_) {
  auto const len = pybind11::len(buf_);
  uint8_t buf[len];
  for (size_t i = 0; i < len; i++) {
    buf[i] = buf_.cast<uint8_t>();
  }
  return pysdk::result(
      WasmEdge_LoaderParseFromBuffer(LoadCxt, ast.get_addr(), buf, len));
}

/* --------------- Loader End -------------------------------- */
