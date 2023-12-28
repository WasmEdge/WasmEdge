#include <common/defines.h>

#if WASMEDGE_OS_MACOS
#include "wasi_random/module.h"
#include <CommonCrypto/CommonRandom.h>

namespace WasmEdge {
namespace Host {

bool WasiRandomEnvironment::getRandomBytes(uint64_t Len, uint8_t *Buf) {
  auto Status = CCRandomGenerateBytes(Buf, Len);
  return Status == kCCSuccess;
}

bool WasiRandomEnvironment::getInsecureRandomBytes(uint64_t Len, uint8_t *Buf) {
  auto Status = CCRandomGenerateBytes(Buf, Len);
  return Status == kCCSuccess;
}
} // namespace Host
} // namespace WasmEdge
#endif
