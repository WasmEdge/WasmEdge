// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "host/wasi_preview2/wasifunc.h"
#include "common/defines.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define __restrict__ __restrict
#endif

namespace WasmEdge {
namespace Host {

using WasmEdge::Span;
using WasmEdge::ValType;
using WasmEdge::ValVariant;

Expect<void> DropPollable::body(const Runtime::CallingFrame &, Pollable This) {
  return Env.dropPollable(This);
}

Expect<ComponentModel::List<bool>>
PollOneoff::body(const Runtime::CallingFrame &Frame, uint32_t PollableListAddr,
                 uint32_t PollableListLen) {
  Runtime::Instance::MemoryInstance *Mem = Frame.getMemoryByIndex(0);
  if (Mem == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto L = ComponentModel::List<Pollable>(PollableListAddr, PollableListLen);
  auto PollableSpan = Env.load<Pollable>(Mem, L);

  std::vector<bool> Result;
  for (auto Pollable : PollableSpan) {
    auto P = Env.getPollable(Pollable);
    if (!P) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    Result.push_back((*P).isReady());
  }

  WasmEdge::Executor::Executor *Exe = Frame.getExecutor();
  auto Mod = Frame.getModule();
  // realloc(0, 0, alignment(tuple_type), size(tuple_type))
  // trap_if(ptr != align_to(ptr, dst_alignment))
  // trap_if(ptr + dst_byte_length > len(cx.opts.memory))
  // encoded = src.encode(dst_encoding)
  // assert(dst_byte_length == len(encoded))
  // cx.opts.memory[ptr : ptr+len(encoded)] = encoded
  // return (ptr, src_code_units)

  auto F = Mod->findFuncExports("realloc");
  // refers to alignment
  // 1.
  // https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md#alignment
  // 2.
  // https://github.com/WebAssembly/component-model/blob/main/design/mvp/CanonicalABI.md#size
  auto Ptr = Exe->invoke(F, Span<const ValVariant>({0, 0, 4, 8}),
                         Span<const ValType>({ValType::I32, ValType::I32,
                                              ValType::I32, ValType::I32}));

  // TODO: understand the above pseudo code and implement it
  return ComponentModel::List<bool>(0, 0);
}

} // namespace Host
} // namespace WasmEdge
