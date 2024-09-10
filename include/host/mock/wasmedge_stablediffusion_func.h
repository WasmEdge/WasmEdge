#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeStableDiffusionMock {

using namespace std::literals;
static inline constexpr const uint32_t kStableDiffusionError = 1U;

class CreateContext : public Runtime::HostFunction<CreateContext> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};

class TextToImage : public Runtime::HostFunction<TextToImage> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, float,
                        uint32_t, uint32_t, int32_t, float, uint32_t, uint32_t,
                        uint32_t, uint32_t, float, float, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};
class ImageToImage : public Runtime::HostFunction<ImageToImage> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, float, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, int32_t, float,
                        uint32_t, uint32_t, float, uint32_t, uint32_t, float,
                        float, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};
class Convert : public Runtime::HostFunction<Convert> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};
} // namespace WasmEdgeStableDiffusionMock
} // namespace Host
} // namespace WasmEdge
