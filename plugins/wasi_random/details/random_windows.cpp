#include <common/defines.h>

#if WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#include "wasi_random/module.h"

namespace WasmEdge {
namespace Host {

static bool WinGetRandomBytes(uint32_t Len, uint8_t *Buf) {
  WasmEdge::winapi::BCRYPT_ALG_HANDLE_ Handle;

  if (unlikely(!BCRYPT_SUCCESS_(WasmEdge::winapi::BCryptOpenAlgorithmProvider(
          &Handle, BCRYPT_RNG_ALGORITHM_, nullptr, 0)))) {
    return false;
  }

  if (unlikely(!BCRYPT_SUCCESS_(
          WasmEdge::winapi::BCryptGenRandom(Handle, Buf, Len, 0)))) {
    return false;
  }

  if (unlikely(!BCRYPT_SUCCESS_(
          WasmEdge::winapi::BCryptCloseAlgorithmProvider(Handle, 0)))) {
    return false;
  }

  return true;
}

bool WasiRandomEnvironment::getRandomBytes(uint64_t Len, uint8_t *Buf) {
  if (Len > UINT32_MAX)
    return false;
  return WinGetRandomBytes(static_cast<uint32_t>(Len), Buf);
}

bool WasiRandomEnvironment::getInsecureRandomBytes(uint64_t Len, uint8_t *Buf) {
  if (Len > UINT32_MAX)
    return false;
  return WinGetRandomBytes(static_cast<uint32_t>(Len), Buf);
}

} // namespace Host
} // namespace WasmEdge
#endif
