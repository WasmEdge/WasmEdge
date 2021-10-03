// SPDX-License-Identifier: Apache-2.0

#include "host/wasi/crypto/ctx.h"

namespace WasmEdge {
namespace Host {
namespace WASI {
namespace Crypto {
HandleMangers::HandleMangers() : ArrayOutputManger{0x00}, OptionsManger{0x01} {}

CryptoCtx::CryptoCtx() {}

WasiCryptoExpect<__wasi_size_t>
CryptoCtx::arrayOutputLen(__wasi_array_output_t ArrayOutputHandle) {
  auto ArrayOutput = Mangers.ArrayOutputManger.get(ArrayOutputHandle);
  return ArrayOutput->len();
}

WasiCryptoExpect<__wasi_size_t>
CryptoCtx::arrayOutputPull(__wasi_array_output_t ArrayOutputHandle,
                           uint8_t_ptr Buf, __wasi_size_t BufLen) {
  auto ArrayOutput = Mangers.ArrayOutputManger.get(ArrayOutputHandle);
  auto Len = ArrayOutput->pull(Span<uint8_t>{reinterpret_cast<uint8_t*>(Buf), BufLen});
  Mangers.ArrayOutputManger.close(ArrayOutputHandle);
  return Len;
}

WasiCryptoExpect<__wasi_options_t>
CryptoCtx::optionsOpen(__wasi_algorithm_type_e_t AlgorithmType) {
  switch (AlgorithmType) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
    break;
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
    break;
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    break;
  }
  return WasmEdge::Host::WASI::Crypto::WasiCryptoExpect<__wasi_options_t>();
}

WasiCryptoExpect<void> CryptoCtx::optionsClose(__wasi_options_t Handle) {
  return Mangers.OptionsManger.close(Handle);
}

WasiCryptoExpect<void> CryptoCtx::optionsSet(__wasi_options_t Handle,
                                             const char *Name,
                                             const_uint8_t_ptr Value,
                                             __wasi_size_t ValueLen) {
  auto Options = Mangers.OptionsManger.get(Handle);
  return Options->set(Name, Span<uint8_t const>{reinterpret_cast<uint8_t const *>(Value), ValueLen});
}

WasiCryptoExpect<void> CryptoCtx::optionsSetU64(__wasi_options_t Handle,
                                                const char *Name,
                                                uint64_t Value) {
  auto Options = Mangers.OptionsManger.get(Handle);
  return Options->setU64(Name, Value);
}

WasiCryptoExpect<void>
CryptoCtx::optionsSetGuestBuffer(__wasi_options_t Handle, const char *Name,
                                 uint8_t_ptr Buffer, __wasi_size_t BufferLen) {
  auto Options = Mangers.OptionsManger.get(Handle);
  return Options->setGuestBuffer(
      Name, Span<uint8_t>{reinterpret_cast<uint8_t *>(Buffer), BufferLen});
}

} // namespace Crypto
} // namespace WASI
} // namespace Host
} // namespace WasmEdge