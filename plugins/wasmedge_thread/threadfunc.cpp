// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadfunc.h"
#include "common/log.h"
#include <iostream>

namespace WasmEdge {
namespace Host {



Expect<uint32_t>
WasmEdgeThreadCreate::body(Runtime::Instance::MemoryInstance *MemInst,
                           uint32_t Thread, [[maybe_unused]] uint32_t Attr,
                           uint32_t StartRoutine, uint32_t Arg) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }
  std::cerr << "WasmEdgeThreadCreate " << StartRoutine << " " << Arg << "\n";

  uint64_t *const WasiThreadPtr =
      MemInst->getPointer<uint64_t *>(Thread);
  if (WasiThreadPtr == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (auto Res = Env.pthreadCreate(WasiThreadPtr, StartRoutine, Arg);
      unlikely(!Res)) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  return {0};
}

Expect<uint32_t>
WasmEdgeThreadJoin::body(Runtime::Instance::MemoryInstance *MemInst,
                         uint32_t Thread, uint32_t Retval) {
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  std::cerr << "WasmEdgeThreadJoin\n";

  void **const WasiRetval = MemInst->getPointer<void **>(Retval);
  if (WasiRetval == nullptr) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  if (auto Res = Env.pthreadJoin(Thread, WasiRetval); unlikely(!Res)) {
    return Unexpect(ErrCode::ExecutionFailed);
  }

  return {0};
}

} // namespace Host
} // namespace WasmEdge
