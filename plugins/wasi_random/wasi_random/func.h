#pragma once

#include "wasi_random/base.h"

namespace WasmEdge {
namespace Host {

class WasiGetRandomBytes : public WasiRandom<WasiGetRandomBytes> {
public:
  WasiGetRandomBytes(WasiRandomEnvironment &HostEnv) : WasiRandom(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint64_t Len,
                    uint32_t /* Out */ BufPtr, uint32_t BufLen);
};

class WasiGetRandomU64 : public WasiRandom<WasiGetRandomU64> {
public:
  WasiGetRandomU64(WasiRandomEnvironment &HostEnv) : WasiRandom(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame,
                    uint32_t /* Out */ U64Ptr);
};

class WasiGetInsecureRandomBytes
    : public WasiRandom<WasiGetInsecureRandomBytes> {
public:
  WasiGetInsecureRandomBytes(WasiRandomEnvironment &HostEnv)
      : WasiRandom(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint64_t Len,
                    uint32_t /* Out */ BufPtr, uint32_t BufLen);
};

class WasiGetInsecureRandomU64 : public WasiRandom<WasiGetInsecureRandomU64> {
public:
  WasiGetInsecureRandomU64(WasiRandomEnvironment &HostEnv)
      : WasiRandom(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame,
                    uint32_t /* Out */ U64Ptr);
};

class WasiInsecureSeed : public WasiRandom<WasiInsecureSeed> {
public:
  WasiInsecureSeed(WasiRandomEnvironment &HostEnv) : WasiRandom(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame,
                    uint32_t /* Out */ U128LoPtr, uint32_t /* Out */ U128HiPtr);
};

} // namespace Host
} // namespace WasmEdge
