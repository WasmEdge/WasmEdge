// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/common.h"
#include "executor/hostfunc.h"
#include "executor/worker/util.h"
#include "vm/envmgr.h"
#include <iostream>

namespace SSVM {
namespace Executor {

#ifdef ONNC_WASM
class QITCTimerStart : public HostFunction {
public:
  QITCTimerStart() { initializeFuncType<QITCTimerStart>(); }

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override {
    return invoke<QITCTimerStart>(EnvMgr, StackMgr, MemInst);
  }

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst) {
    EnvMgr.getTimeRecorder().startRecord(TIMER_TAG_QITC_INFER_SSVM);
    EnvMgr.IsQITCTimer = true;
    return ErrCode::Success;
  }
};

class QITCTimerStop : public HostFunction {
public:
  QITCTimerStop() { initializeFuncType<QITCTimerStop>(); }

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override {
    return invoke<QITCTimerStop>(EnvMgr, StackMgr, MemInst);
  }

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst) {
    uint64_t SSVMTime =
        EnvMgr.getTimeRecorder().stopRecord(TIMER_TAG_QITC_INFER_SSVM);
    uint64_t HostTime =
        EnvMgr.getTimeRecorder().stopRecord(TIMER_TAG_QITC_INFER_HOST);
    std::cout << " --- Inference: SSVM cost " << SSVMTime
              << " us, Host functions cost " << HostTime << " us\n";
    EnvMgr.IsQITCTimer = false;
    return ErrCode::Success;
  }
};

class QITCTimerClear : public HostFunction {
public:
  QITCTimerClear() { initializeFuncType<QITCTimerClear>(); }

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override {
    return invoke<QITCTimerClear>(EnvMgr, StackMgr, MemInst);
  }

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst) {
    EnvMgr.getTimeRecorder().clearRecord(TIMER_TAG_QITC_INFER_SSVM);
    EnvMgr.getTimeRecorder().clearRecord(TIMER_TAG_QITC_INFER_HOST);
    EnvMgr.IsQITCTimer = false;
    return ErrCode::Success;
  }
};
#endif

} // namespace Executor
} // namespace SSVM
