// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "processbase.h"
#include "processenv.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"

namespace SSVM {
namespace Host {

class SSVMProcessSetProgName : public SSVMProcess<SSVMProcessSetProgName> {
public:
  SSVMProcessSetProgName(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t NamePtr, uint32_t NameLen);
};

class SSVMProcessAddArg : public SSVMProcess<SSVMProcessAddArg> {
public:
  SSVMProcessAddArg(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t ArgPtr,
                    uint32_t ArgLen);
};

class SSVMProcessAddEnv : public SSVMProcess<SSVMProcessAddEnv> {
public:
  SSVMProcessAddEnv(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t EnvNamePtr, uint32_t EnvNameLen,
                    uint32_t EnvValPtr, uint32_t EnvValLen);
};

class SSVMProcessAddStdIn : public SSVMProcess<SSVMProcessAddStdIn> {
public:
  SSVMProcessAddStdIn(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t BufPtr,
                    uint32_t BufLen);
};

class SSVMProcessSetTimeOut : public SSVMProcess<SSVMProcessSetTimeOut> {
public:
  SSVMProcessSetTimeOut(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Time);
};

class SSVMProcessRun : public SSVMProcess<SSVMProcessRun> {
public:
  SSVMProcessRun(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class SSVMProcessGetExitCode : public SSVMProcess<SSVMProcessGetExitCode> {
public:
  SSVMProcessGetExitCode(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class SSVMProcessGetStdOutLen : public SSVMProcess<SSVMProcessGetStdOutLen> {
public:
  SSVMProcessGetStdOutLen(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class SSVMProcessGetStdOut : public SSVMProcess<SSVMProcessGetStdOut> {
public:
  SSVMProcessGetStdOut(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t BufPtr);
};

class SSVMProcessGetStdErrLen : public SSVMProcess<SSVMProcessGetStdErrLen> {
public:
  SSVMProcessGetStdErrLen(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class SSVMProcessGetStdErr : public SSVMProcess<SSVMProcessGetStdErr> {
public:
  SSVMProcessGetStdErr(SSVMProcessEnvironment &Env) : SSVMProcess(Env) {}
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t BufPtr);
};

} // namespace Host
} // namespace SSVM
