// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/common.h"
#include "executor/hostfunc.h"
#include "executor/worker/util.h"
#include "vm/envmgr.h"

#include <iostream>
#include <string>

namespace SSVM {
namespace Executor {

#ifdef ONNC_WASM
class QITCTimerStart : public HostFunction {
public:
  QITCTimerStart() = default;
  virtual ~QITCTimerStart() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst) {
    EnvMgr.getTimeRecorder().startRecord(TIMER_TAG_QITC_INFER_SSVM);
    EnvMgr.IsQITCTimer = true;
    return ErrCode::Success;
  }
};

class QITCTimerStop : public HostFunction {
public:
  QITCTimerStop() = default;
  virtual ~QITCTimerStop() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst) {
    uint64_t SSVMTime =
        EnvMgr.getTimeRecorder().stopRecord(TIMER_TAG_QITC_INFER_SSVM);
    uint64_t HostTime =
        EnvMgr.getTimeRecorder().stopRecord(TIMER_TAG_QITC_INFER_HOST);
    printf(" --- Inference: SSVM cost %llu us, Host functions cost %llu us\n",
           SSVMTime, HostTime);
    EnvMgr.IsQITCTimer = false;
    return ErrCode::Success;
  }
};

class QITCTimerClear : public HostFunction {
public:
  QITCTimerClear() = default;
  virtual ~QITCTimerClear() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst) {
    EnvMgr.getTimeRecorder().clearRecord(TIMER_TAG_QITC_INFER_SSVM);
    EnvMgr.getTimeRecorder().clearRecord(TIMER_TAG_QITC_INFER_HOST);
    EnvMgr.IsQITCTimer = false;
    return ErrCode::Success;
  }
};
#endif

} // namespace Executor
} // namespace SSVM
