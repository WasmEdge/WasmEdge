// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include "support/time.h"

#include <iostream>

#define TIMER_TAG_QITC_INFER_SSVM 256U
#define TIMER_TAG_QITC_INFER_HOST 257U

namespace SSVM {
namespace Host {

template <typename T> class QITC : public Runtime::HostFunction<T> {
public:
  QITC(Support::TimeRecord &R) : Runtime::HostFunction<T>(0), Timer(R) {}

protected:
  Support::TimeRecord &Timer;
};

class QITCTimerStart : public QITC<QITCTimerStart> {
public:
  QITCTimerStart(Support::TimeRecord &T) : QITC<QITCTimerStart>(T) {}
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst) {
    Timer.startRecord(TIMER_TAG_QITC_INFER_SSVM);
    /// TODO: Inject timer in measurement to start to record ssvm and host time.
    return ErrCode::Success;
  }
};

class QITCTimerStop : public QITC<QITCTimerStop> {
public:
  QITCTimerStop(Support::TimeRecord &T) : QITC<QITCTimerStop>(T) {}
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst) {
    uint64_t SSVMTime = Timer.stopRecord(TIMER_TAG_QITC_INFER_SSVM);
    uint64_t HostTime = Timer.stopRecord(TIMER_TAG_QITC_INFER_HOST);
    std::cout << " --- Inference: SSVM cost " << SSVMTime
              << " us, Host functions cost " << HostTime << " us\n";
    /// TODO: Inject timer in measurement to stop recording ssvm and host time.
    return ErrCode::Success;
  }
};

class QITCTimerClear : public QITC<QITCTimerClear> {
public:
  QITCTimerClear(Support::TimeRecord &T) : QITC<QITCTimerClear>(T) {}
  ErrCode body(Runtime::Instance::MemoryInstance &MemInst) {
    Timer.clearRecord(TIMER_TAG_QITC_INFER_SSVM);
    Timer.clearRecord(TIMER_TAG_QITC_INFER_HOST);
    /// TODO: Inject timer in measurement to stop recording ssvm and host time.
    return ErrCode::Success;
  }
};

class QITCModule : public Runtime::ImportObject {
public:
  QITCModule() : ImportObject("QITC") {
    addHostFunc("QITC_time_start", std::make_unique<QITCTimerStart>(Timer));
    addHostFunc("QITC_time_stop", std::make_unique<QITCTimerStop>(Timer));
    addHostFunc("QITC_time_clear", std::make_unique<QITCTimerClear>(Timer));
  }
  virtual ~QITCModule() = default;

private:
  Support::TimeRecord Timer;
};

} // namespace Host
} // namespace SSVM
