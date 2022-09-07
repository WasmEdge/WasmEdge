// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadfunc.h"
#include "common/log.h"
#include <iostream>

namespace WasmEdge {
namespace Host {

Expect<uint32_t> WasmEdgeThreadCreate::body(const Runtime::CallingFrame &Frame,
                                            uint32_t Thread,
                                            [[maybe_unused]] uint32_t Attr,
                                            uint32_t StartRoutine,
                                            uint32_t Arg) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::cerr << "WasmEdgeThreadCreate " << StartRoutine << " " << Arg << "\n";

  uint32_t *WasiThreadPtr = MemInst->getPointer<uint32_t *>(Thread);
  if (WasiThreadPtr == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (auto Res = Env.threadCreate(Frame.getExecutor(), StartRoutine, Arg);
      unlikely(!Res)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  } else {
    std::cerr << "create thread with tid " << (*Res) << "\n";
    *WasiThreadPtr = *Res;
  }

  return {0};
}

Expect<uint32_t> WasmEdgeThreadJoin::body(const Runtime::CallingFrame &Frame,
                                          uint32_t Tid, uint32_t Retval) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  std::cerr << "WasmEdgeThreadJoin " << Tid << "\n";

  void **const WasiRetval = MemInst->getPointer<void **>(Retval);
  if (WasiRetval == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (auto Res = Env.threadJoin(Tid, WasiRetval); unlikely(!Res)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  } else {
    // *WasiRetval = *Res;
    return {0};
  }
}

} // namespace Host
} // namespace WasmEdge