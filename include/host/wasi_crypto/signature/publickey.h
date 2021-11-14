// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi_crypto/error.h"

#include <memory>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignaturePublicKey {
public:
  static void import();
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
