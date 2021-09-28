// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/crypto/handles.h"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {

struct HandleMangers {
  HandleMangers();
};

class CryptoCtx {
public:
  CryptoCtx();
private:
  HandleMangers Mangers;
};

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge