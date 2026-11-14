// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "helper.h"

#include "common/defines.h"
#include "common/filesystem.h"
#include "plugin/plugin.h"
#include "po/argument_parser.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {
using namespace std::literals;

namespace {

template <typename T, typename U>
inline std::unique_ptr<T> dynamicPointerCast(std::unique_ptr<U> &&R) noexcept {
  static_assert(std::has_virtual_destructor_v<T>);
  T *P = dynamic_cast<T *>(R.get());
  if (P) {
    R.release();
  }
  return std::unique_ptr<T>(P);
}

} // namespace

std::unique_ptr<WasiNNModule> createModule(std::string_view NNRPCURI) {
  WasmEdge::Plugin::Plugin::load(
      WasmEdge::u8path("../../../plugins/wasi_nn/" WASMEDGE_LIB_PREFIX
                       "wasmedgePluginWasiNN" WASMEDGE_LIB_EXTENSION));
  if (const auto *Plugin = WasmEdge::Plugin::Plugin::find("wasi_nn"sv)) {
    WasmEdge::PO::ArgumentParser Parser;
    Plugin->registerOptions(Parser);
    if (NNRPCURI != "") {
      Parser.set_raw_value<std::string>("nn-rpc-uri"sv, std::string(NNRPCURI));
    }
    if (const auto *Module = Plugin->findModule("wasi_nn"sv)) {
      return dynamicPointerCast<WasiNNModule>(Module->create());
    }
  }
  return {};
}

std::vector<uint8_t> readEntireFile(const std::string &Path) {
  std::ifstream Fin(Path, std::ios::in | std::ios::binary | std::ios::ate);
  if (!Fin) {
    return {};
  }
  std::vector<uint8_t> Buf(static_cast<std::size_t>(Fin.tellg()));
  Fin.seekg(0, std::ios::beg);
  if (!Fin.read(reinterpret_cast<char *>(Buf.data()),
                static_cast<std::streamsize>(Buf.size()))) {
    return {};
  }
  Fin.close();
  return Buf;
}

void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 uint32_t Value, uint32_t &Ptr) {
  MemInst.storeValue(Value, Ptr);
  Ptr += 4;
}

void writeFatPointer(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t PtrVal, uint32_t PtrSize, uint32_t &Ptr) {
  writeUInt32(MemInst, PtrVal, Ptr);
  writeUInt32(MemInst, PtrSize, Ptr);
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
