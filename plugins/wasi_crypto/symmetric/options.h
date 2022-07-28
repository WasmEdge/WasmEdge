// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/options.h - Options --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the Symmetric Options class definition.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/error.h"

#include "common/span.h"

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

/// Options for symmetric state and key.
///
/// More detail:
/// https://github.com/WebAssembly/wasi-crypto/blob/main/docs/wasi-crypto.md#options-1
class Options {
public:
  WasiCryptoExpect<void> set(std::string_view Name,
                             Span<const uint8_t> Value) noexcept;

  WasiCryptoExpect<void> setU64(std::string_view Name, uint64_t Value) noexcept;

  WasiCryptoExpect<void> setGuestBuffer(std::string_view Name,
                                        Span<uint8_t> Buffer) noexcept;

  WasiCryptoExpect<size_t> get(std::string_view Name,
                               Span<uint8_t> Value) const noexcept;

  WasiCryptoExpect<uint64_t> getU64(std::string_view Name) const noexcept;

private:
  struct DataType {
    std::map<std::string, std::vector<uint8_t>> ValueMap;
    std::map<std::string, uint64_t> U64ValueMap;
    std::optional<Span<uint8_t>> GuestBuffer;
    mutable std::shared_mutex Mutex;
  };

  std::shared_ptr<DataType> Inner = std::make_shared<DataType>();
};

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
