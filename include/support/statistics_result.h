// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "easyloggingpp/easylogging++.h"
#include "executor/common.h"

#include <string>

namespace SSVM {
namespace Support {

class Result {
public:
  virtual void show() = 0;
};

class ExeResult : public Result {
public:
  ExeResult(Executor::ErrCode Status, std::string Information, uint64_t ExecTime = 0, uint64_t HostFuncTime = 0, uint64_t ExecInstrCnt = 0, uint64_t Gas = 0):
    Status(Status),
    Information(Information),
    ExecTime(ExecTime),
    HostFuncTime(HostFuncTime),
    ExecInstrCnt(ExecInstrCnt),
    Gas(Gas) {}

  virtual void show() {
    switch (Status) {
        case Executor::ErrCode::Success:
          LOG(INFO) << "Worker execution succeeded.";
          break;
        case Executor::ErrCode::Revert:
          LOG(ERROR) << "Reverted.";
          break;
        case Executor::ErrCode::Terminated:
          LOG(ERROR) << "Terminated.";
          break;
        default:
          LOG(ERROR)  << "Worker execution failed. Code: "
                      << (unsigned int)Status;
        }

        LOG(INFO) << std::endl
              << " =================  Statistics  =================" << std::endl
              << " Total execution time: " << ExecTime + HostFuncTime << " us"
              << std::endl
              << " Wasm instructions execution time: " << ExecTime << " us"
              << std::endl
              << " Host functions execution time: " << HostFuncTime << " us"
              << std::endl
              << " Executed wasm instructions count: " << ExecInstrCnt
              << std::endl
        #ifndef ONNC_WASM
              << " Gas costs: " << Gas << std::endl
        #endif
              << " Instructions per second: "
              << static_cast<uint64_t>((double)ExecInstrCnt * 1000000 / ((ExecTime)?ExecTime:1) )
              << std::endl;
  }
private:
  Executor::ErrCode Status;
  std::string Information;
  uint64_t ExecTime;
  uint64_t HostFuncTime;
  uint64_t ExecInstrCnt;
  uint64_t Gas;
};

} // namespace Support
} // namespace SSVM
