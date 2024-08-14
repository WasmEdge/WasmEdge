// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/loader/aot_section.h - AOT Section definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the AOTSection, which holds logics to
/// load from an AOTSection
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/section.h"
#include "common/executable.h"
#include "common/filesystem.h"
#include "system/winapi.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Loader {

/// Holder class for library handle
class AOTSection : public Executable {
public:
  AOTSection() noexcept = default;
  ~AOTSection() noexcept override { unload(); }
  Expect<void> load(const AST::AOTSection &AOTSec) noexcept;
  void unload() noexcept;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override {
    if (Binary) {
      return createSymbol<const IntrinsicsTable *>(
          getPointer<const IntrinsicsTable *>(IntrinsicsAddress));
    }
    return {};
  }

  std::vector<Symbol<Wrapper>> getTypes(size_t) noexcept override {
    std::vector<Symbol<Wrapper>> Result;
    if (Binary) {
      Result.reserve(TypesAddress.size());
      for (const auto Address : TypesAddress) {
        Result.push_back(createSymbol<Wrapper>(getPointer<Wrapper>(Address)));
      }
    }
    return Result;
  }

  std::vector<Symbol<void>> getCodes(size_t, size_t) noexcept override {
    std::vector<Symbol<void>> Result;
    if (Binary) {
      Result.reserve(CodesAddress.size());
      for (const auto Address : CodesAddress) {
        Result.push_back(createSymbol<void>(getPointer<void>(Address)));
      }
    }
    return Result;
  }

private:
  uintptr_t getOffset() const noexcept {
    return reinterpret_cast<uintptr_t>(Binary);
  }

  template <typename T> T *getPointer(uint64_t Address) const noexcept {
    return reinterpret_cast<T *>(getOffset() + Address);
  }

  uint8_t *Binary = nullptr;
  uint64_t BinarySize = 0;
  uint64_t IntrinsicsAddress = 0;
  std::vector<uintptr_t> TypesAddress;
  std::vector<uintptr_t> CodesAddress;
#if WASMEDGE_OS_LINUX
  void *EHFrameAddress = nullptr;
#elif WASMEDGE_OS_MACOS
  uint8_t *EHFrameAddress = nullptr;
  uint32_t EHFrameSize = 0;
#elif WASMEDGE_OS_WINDOWS
  void *PDataAddress = nullptr;
  uint32_t PDataSize = 0;
#endif
};

} // namespace Loader
} // namespace WasmEdge
