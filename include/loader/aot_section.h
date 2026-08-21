// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/loader/aot_section.h - AOT Section definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the AOTSection, which contains the
/// logic for loading an AOT section.
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

/// Holder class for a library handle.
class AOTSection : public Executable {
public:
  AOTSection() noexcept = default;
  ~AOTSection() noexcept override { unload(); }
  Expect<void> load(const AST::AOTSection &AOTSec) noexcept;
  void unload() noexcept;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override {
    if (Binary) {
      if (auto *const Pointer = getPointer<const IntrinsicsTable *>(
              IntrinsicsAddress, sizeof(const IntrinsicsTable *))) {
        return createSymbol<const IntrinsicsTable *>(Pointer);
      }
    }
    return {};
  }

  std::vector<Symbol<Wrapper>> getTypes(size_t) noexcept override {
    std::vector<Symbol<Wrapper>> Result;
    if (Binary) {
      Result.reserve(TypesAddress.size());
      for (const auto Address : TypesAddress) {
        auto *const Pointer = getPointer<Wrapper>(Address, 1);
        if (!Pointer) {
          return {};
        }
        Result.push_back(createSymbol<Wrapper>(Pointer));
      }
    }
    return Result;
  }

  std::vector<Symbol<void>> getCodes(size_t, size_t) noexcept override {
    std::vector<Symbol<void>> Result;
    if (Binary) {
      Result.reserve(CodesAddress.size());
      for (const auto Address : CodesAddress) {
        auto *const Pointer = getPointer<void>(Address, 1);
        if (!Pointer) {
          return {};
        }
        Result.push_back(createSymbol<void>(Pointer));
      }
    }
    return Result;
  }

private:
  uintptr_t getOffset() const noexcept {
    return reinterpret_cast<uintptr_t>(Binary);
  }

  /// Check that [Offset, Offset + Length) stays inside the mapped binary.
  bool checkAccessBound(uint64_t Offset, uint64_t Length) const noexcept {
    return Offset <= BinarySize && Length <= BinarySize - Offset;
  }

  template <typename T>
  T *getPointer(uint64_t Address, uint64_t Size) const noexcept {
    if (!checkAccessBound(Address, Size)) {
      return nullptr;
    }
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
