#include "WasmEdge.hpp"

/* --------------- Loader -------------------------------- */

pysdk::Loader::Loader(pysdk::Configure &cfg) {
  LoadCxt = WasmEdge_LoaderCreate(cfg.get());
}

pysdk::Loader::~Loader() { WasmEdge_LoaderDelete(LoadCxt); }

WasmEdge_LoaderContext *pysdk::Loader::get() { return LoadCxt; }

pysdk::result pysdk::Loader::parse(pysdk::ASTModuleCxt &ast,
                                   std::string &path) {
  auto ast_ = ast.get();
  return pysdk::result(
      WasmEdge_LoaderParseFromFile(LoadCxt, &ast_, path.c_str()));
}

pysdk::result pysdk::Loader::parse(pysdk::ASTModuleCxt &ast,
                                   pybind11::tuple buf_) {
  auto const len = pybind11::len(buf_);
  uint8_t buf[len];
  for (size_t i = 0; i < len; i++) {
    buf[i] = buf_.cast<uint8_t>();
  }

  auto ast_ = ast.get();
  return pysdk::result(
      WasmEdge_LoaderParseFromBuffer(LoadCxt, &ast_, buf, len));
}

/* --------------- Loader End -------------------------------- */
