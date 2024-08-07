// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "processbase.h"

#include "runtime/callingframe.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessSetProgName
    : public WasmEdgeProcess<WasmEdgeProcessSetProgName> {
public:
  WasmEdgeProcessSetProgName(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t NamePtr,
                    uint32_t NameLen);
};

class WasmEdgeProcessAddArg : public WasmEdgeProcess<WasmEdgeProcessAddArg> {
public:
  WasmEdgeProcessAddArg(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t ArgPtr,
                    uint32_t ArgLen);
};

class WasmEdgeProcessAddEnv : public WasmEdgeProcess<WasmEdgeProcessAddEnv> {
public:
  WasmEdgeProcessAddEnv(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t EnvNamePtr,
                    uint32_t EnvNameLen, uint32_t EnvValPtr,
                    uint32_t EnvValLen);
};

class WasmEdgeProcessAddStdIn
    : public WasmEdgeProcess<WasmEdgeProcessAddStdIn> {
public:
  WasmEdgeProcessAddStdIn(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                    uint32_t BufLen);
};

class WasmEdgeProcessSetTimeOut
    : public WasmEdgeProcess<WasmEdgeProcessSetTimeOut> {
public:
  WasmEdgeProcessSetTimeOut(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Time);
};

class WasmEdgeProcessRun : public WasmEdgeProcess<WasmEdgeProcessRun> {
public:
  WasmEdgeProcessRun(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class WasmEdgeProcessGetExitCode
    : public WasmEdgeProcess<WasmEdgeProcessGetExitCode> {
public:
  WasmEdgeProcessGetExitCode(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class WasmEdgeProcessGetStdOutLen
    : public WasmEdgeProcess<WasmEdgeProcessGetStdOutLen> {
public:
  WasmEdgeProcessGetStdOutLen(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class WasmEdgeProcessGetStdOut
    : public WasmEdgeProcess<WasmEdgeProcessGetStdOut> {
public:
  WasmEdgeProcessGetStdOut(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr);
};

class WasmEdgeProcessGetStdErrLen
    : public WasmEdgeProcess<WasmEdgeProcessGetStdErrLen> {
public:
  WasmEdgeProcessGetStdErrLen(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class WasmEdgeProcessGetStdErr
    : public WasmEdgeProcess<WasmEdgeProcessGetStdErr> {
public:
  WasmEdgeProcessGetStdErr(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr);
};

} // namespace Host
} // namespace WasmEdge
