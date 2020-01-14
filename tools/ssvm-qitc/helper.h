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
class QITCTimerStart : public HostFunction<QITCTimerStart> {
public:
  QITCTimerStart()
      : HostFunction<QITCTimerStart>("QITC", "QITC_time_start", 0) {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst) {
    EnvMgr.getTimeRecorder().startRecord(TIMER_TAG_QITC_INFER_SSVM);
    EnvMgr.IsQITCTimer = true;
    return ErrCode::Success;
  }
};

class QITCTimerStop : public HostFunction<QITCTimerStop> {
public:
  QITCTimerStop() : HostFunction<QITCTimerStop>("QITC", "QITC_time_stop", 0) {}
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

class QITCTimerClear : public HostFunction<QITCTimerClear> {
public:
  QITCTimerClear()
      : HostFunction<QITCTimerClear>("QITC", "QITC_time_clear", 0) {}
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
