// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "zlibbase.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibDeflateInit_ : public WasmEdgeZlib<WasmEdgeZlibDeflateInit_> {
public:
  WasmEdgeZlibDeflateInit_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Level, uint32_t VersionPtr, uint32_t StreamSize);
};

class WasmEdgeZlibInflateInit_ : public WasmEdgeZlib<WasmEdgeZlibInflateInit_> {
public:
  WasmEdgeZlibInflateInit_(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       uint32_t VersionPtr, uint32_t StreamSize);
};

class WasmEdgeZlibDeflate : public WasmEdgeZlib<WasmEdgeZlibDeflate> {
public:
  WasmEdgeZlibDeflate(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Flush);
};

class WasmEdgeZlibInflate : public WasmEdgeZlib<WasmEdgeZlibInflate> {
public:
  WasmEdgeZlibInflate(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr,
                       int32_t Flush);
};

class WasmEdgeZlibDeflateEnd : public WasmEdgeZlib<WasmEdgeZlibDeflateEnd> {
public:
  WasmEdgeZlibDeflateEnd(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

class WasmEdgeZlibInflateEnd : public WasmEdgeZlib<WasmEdgeZlibInflateEnd> {
public:
  WasmEdgeZlibInflateEnd(WasmEdgeZlibEnvironment &HostEnv)
      : WasmEdgeZlib(HostEnv) {}
  Expect<int32_t> body(const Runtime::CallingFrame &Frame, uint32_t ZStreamPtr);
};

} // namespace Host
} // namespace WasmEdge
