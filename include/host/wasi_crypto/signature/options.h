// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/options.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SignatureOptions : public Common::Options {
public:
  using Common::Options::Options;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
