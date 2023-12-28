#pragma once

#include "plugin/plugin.h"

namespace WasmEdge {
namespace Host {

class WasiRandomEnvironment {
public:
  WasiRandomEnvironment() noexcept {}

  // getRandomBytes is a nonblock CSPRNG. The function return the
  // operation is success or not, and fill the Buf with random bytes
  bool getRandomBytes(uint64_t Len, uint8_t *Buf);
  bool getInsecureRandomBytes(uint64_t Len, uint8_t *Buf);
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
