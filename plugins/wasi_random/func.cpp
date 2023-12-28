#include "wasi_random/func.h"
#include "common/errcode.h"
#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals;

Expect<void> WasiGetRandomBytes::body(const Runtime::CallingFrame &Frame,
                                      uint64_t Len, uint32_t /* Out */ BufPtr,
                                      uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (unlikely(MemInst == nullptr)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check for invalid address.
  const auto Buf = MemInst->getSpan<uint8_t>(BufPtr, BufLen);
  if (unlikely(Buf.size() != BufLen)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Len > BufLen)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Env.getRandomBytes(Len, Buf.data()) == false)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

Expect<void> WasiGetRandomU64::body(const Runtime::CallingFrame &Frame,
                                    uint32_t /* Out */ U64Ptr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (unlikely(MemInst == nullptr)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check for invalid address.
  uint32_t U64Len = sizeof(uint64_t);
  const auto Buf = MemInst->getSpan<uint8_t>(U64Ptr, U64Len);
  if (unlikely(Buf.size() != U64Len)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Env.getRandomBytes(U64Len, Buf.data()) == false)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

Expect<void>
WasiGetInsecureRandomBytes::body(const Runtime::CallingFrame &Frame,
                                 uint64_t Len, uint32_t /* Out */ BufPtr,
                                 uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (unlikely(MemInst == nullptr)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check for invalid address.
  const auto Buf = MemInst->getSpan<uint8_t>(BufPtr, BufLen);
  if (unlikely(Buf.size() != BufLen)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Len > BufLen)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Env.getRandomBytes(Len, Buf.data()) == false)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

Expect<void> WasiGetInsecureRandomU64::body(const Runtime::CallingFrame &Frame,
                                            uint32_t /* Out */ U64Ptr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (unlikely(MemInst == nullptr)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check for invalid address.
  constexpr auto U64Len = sizeof(uint64_t);
  const auto Buf = MemInst->getSpan<uint8_t>(U64Ptr, U64Len);
  if (unlikely(Buf.size() != U64Len)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Env.getRandomBytes(U64Len, Buf.data()) == false)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

Expect<void> WasiInsecureSeed::body(const Runtime::CallingFrame &Frame,
                                    uint32_t /* Out */ U128LoPtr,
                                    uint32_t /* Out */ U128HiPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (unlikely(MemInst == nullptr)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Check for invalid address.
  uint32_t U64Len = sizeof(uint64_t);
  auto BufLo = MemInst->getSpan<uint8_t>(U128LoPtr, U64Len);
  if (unlikely(BufLo.size() != U64Len)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto BufHi = MemInst->getSpan<uint8_t>(U128HiPtr, U64Len);
  if (unlikely(BufHi.size() != U64Len)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (unlikely(Env.getInsecureRandomBytes(U64Len, BufLo.data()) == false ||
               Env.getInsecureRandomBytes(U64Len, BufHi.data()) == false)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return {};
}

} // namespace Host
} // namespace WasmEdge
