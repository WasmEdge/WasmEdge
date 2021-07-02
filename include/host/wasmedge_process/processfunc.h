// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "processbase.h"
#include "processenv.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessSetProgName
    : public WasmEdgeProcess<WasmEdgeProcessSetProgName> {
public:
  WasmEdgeProcessSetProgName(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t NamePtr, uint32_t NameLen);
};

class WasmEdgeProcessAddArg : public WasmEdgeProcess<WasmEdgeProcessAddArg> {
public:
  WasmEdgeProcessAddArg(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t ArgPtr,
                    uint32_t ArgLen);
};

class WasmEdgeProcessAddEnv : public WasmEdgeProcess<WasmEdgeProcessAddEnv> {
public:
  WasmEdgeProcessAddEnv(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t EnvNamePtr, uint32_t EnvNameLen,
                    uint32_t EnvValPtr, uint32_t EnvValLen);
};

class WasmEdgeProcessAddStdIn
    : public WasmEdgeProcess<WasmEdgeProcessAddStdIn> {
public:
  WasmEdgeProcessAddStdIn(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t BufPtr,
                    uint32_t BufLen);
};

class WasmEdgeProcessSetTimeOut
    : public WasmEdgeProcess<WasmEdgeProcessSetTimeOut> {
public:
  WasmEdgeProcessSetTimeOut(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Time);
};

class WasmEdgeProcessRun : public WasmEdgeProcess<WasmEdgeProcessRun> {
public:
  WasmEdgeProcessRun(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class WasmEdgeProcessGetExitCode
    : public WasmEdgeProcess<WasmEdgeProcessGetExitCode> {
public:
  WasmEdgeProcessGetExitCode(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class WasmEdgeProcessGetStdOutLen
    : public WasmEdgeProcess<WasmEdgeProcessGetStdOutLen> {
public:
  WasmEdgeProcessGetStdOutLen(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class WasmEdgeProcessGetStdOut
    : public WasmEdgeProcess<WasmEdgeProcessGetStdOut> {
public:
  WasmEdgeProcessGetStdOut(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t BufPtr);
};

class WasmEdgeProcessGetStdErrLen
    : public WasmEdgeProcess<WasmEdgeProcessGetStdErrLen> {
public:
  WasmEdgeProcessGetStdErrLen(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class WasmEdgeProcessGetStdErr
    : public WasmEdgeProcess<WasmEdgeProcessGetStdErr> {
public:
  WasmEdgeProcessGetStdErr(WasmEdgeProcessEnvironment &HostEnv)
      : WasmEdgeProcess(HostEnv) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t BufPtr);
};

} // namespace Host
} // namespace WasmEdge
