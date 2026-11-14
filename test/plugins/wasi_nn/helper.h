// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinnmodule.h"

#include "common/span.h"
#include "runtime/instance/memory.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASINN {

std::unique_ptr<WasiNNModule> createModule(std::string_view NNRPCURI = "");

std::vector<uint8_t> readEntireFile(const std::string &Path);

template <typename T>
void writeBinaries(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   WasmEdge::Span<const T> Binaries, uint32_t Ptr) noexcept {
  std::copy(Binaries.begin(), Binaries.end(), MemInst.getPointer<T *>(Ptr));
}

void writeUInt32(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 uint32_t Value, uint32_t &Ptr);

void writeFatPointer(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                     uint32_t PtrVal, uint32_t PtrSize, uint32_t &Ptr);

template <typename T>
std::vector<size_t> classSort(WasmEdge::Span<const T> Array) {
  std::vector<size_t> Indices(Array.size());
  std::iota(Indices.begin(), Indices.end(), 0);
  std::sort(Indices.begin(), Indices.end(),
            [&Array](size_t Left, size_t Right) -> bool {
              // Sort indices according to the corresponding array elements.
              return Array[Left] > Array[Right];
            });
  return Indices;
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
