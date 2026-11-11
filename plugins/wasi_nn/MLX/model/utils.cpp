// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "model/utils.h"
#include "host/filesystem.h"
#include <cstdint>
#include <fstream>
#include <new>
#include <sstream>

namespace WasmEdge::Host::WASINN::MLX {

namespace {

constexpr __wasi_filesize_t MaxFileSize = UINT32_C(1) << 30;

} // namespace

std::vector<std::string> splitString(const std::string &S, char Delim) {
  std::vector<std::string> Result;
  std::stringstream SS(S);
  std::string Item;
  while (std::getline(SS, Item, Delim)) {
    Result.emplace_back(Item);
  }
  return Result;
}

std::string joinString(std::vector<std::string> &S, char Delim) {
  std::string Result;
  for (size_t Idx = 0; Idx < S.size(); Idx++) {
    Result += S[Idx];
    if (Idx != S.size() - 1) {
      Result += Delim;
    }
  }
  return Result;
}

bool endsWith(std::string const &Value, std::string const &Ending) {
  if (Ending.size() > Value.size())
    return false;
  return std::equal(Ending.rbegin(), Ending.rend(), Value.rbegin());
}

bool startsWith(std::string const &Value, std::string const &Starting) {
  if (Starting.size() > Value.size())
    return false;
  return std::equal(Starting.begin(), Starting.end(), Value.begin());
}

void saveWeights(const std::unordered_map<std::string, mx::array> &Weights,
                 const std::string Path) {
  if (endsWith(Path, ".safetensors")) {
    mx::save_safetensors(Path, Weights, {{"format", "mlx"}});
  } else {
    spdlog::error("[WASI-NN] MLX backend: Unsupported file format"sv);
    assumingUnreachable();
  }
}

void saveWeights(const mx::array &Weights, const std::string &Path) {
  if (endsWith(Path, ".npz")) {
    mx::save(Path, Weights);
  } else {
    spdlog::error("[WASI-NN] MLX backend: Unsupported file format"sv);
    assumingUnreachable();
  }
}

std::string loadBytesFromFile(const std::string &Path,
                              const Host::FileSystem *FS) {
  WASI::WasiExpect<std::shared_ptr<Host::File>> File =
      WASI::WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  if (FS != nullptr) {
    File = FS->open(Path, static_cast<__wasi_oflags_t>(0),
                    __WASI_RIGHTS_FD_READ, static_cast<__wasi_fdflags_t>(0));
  }
  if (!File) {
    spdlog::error("[WASI-NN] MLX backend: Cannot open {}."sv, Path);
    return "";
  }
  // The size of the file is the offset of its end.
  std::string Data;
  auto Size = (*File)->seek(0, __WASI_WHENCE_END);
  if (!Size || *Size > MaxFileSize || !(*File)->seek(0, __WASI_WHENCE_SET)) {
    spdlog::error("[WASI-NN] MLX backend: Cannot read {}."sv, Path);
    return Data;
  }
  try {
    Data.resize(static_cast<size_t>(*Size));
  } catch (const std::bad_alloc &) {
    spdlog::error("[WASI-NN] MLX backend: Cannot read {}."sv, Path);
    return Data;
  }
  auto ReadSize = (*File)->read(
      Span<uint8_t>(reinterpret_cast<uint8_t *>(Data.data()), Data.size()));
  Data.resize(ReadSize ? *ReadSize : 0);
  return Data;
}

} // namespace WasmEdge::Host::WASINN::MLX
