// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/log.h"
#include "executor/executor.h"
#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/ip-name-lookup/wasifunc.h"
#include "host/preview2/wasi-sockets/resource_table.h"
#include "host/preview2/wasi-sockets/util.h"
#include "host/wasi/environ.h"
#include "runtime/instance/memory.h"
#include <atomic>
#include <deque>

namespace WasmEdge {
namespace Host {
namespace WasiSocket {
namespace IpNameLookup {

static void WasiSocketsResolveAddressesBlock(WasiSocketsEnvironment &Env,
                                             ResolveAddressStream &Data) {
  do {
    auto Res = Env.sockGetaddrinfoPV2(Data.Query, Data.Resolved);
    if (!Res) {
      Data.Invalid = true;
    }
  } while (false);
  Data.IsReady = true;
};

Expect<void> ResolveAddresses::body(const Runtime::CallingFrame &Frame,
                                    uint32_t Handle, uint32_t StrPtr,
                                    uint32_t StrLen, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  (void)Handle;

  auto RetBuf = MemInst->getPointer<
      __wasi_sockets_ret_t<__wasi_resolve_address_stream_handle_t> *>(
      RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  std::string_view Query = MemInst->getStringView(StrPtr, StrLen);

  // TODO: check access
  auto ResHandle =
      Env.Table.Push<ResolveAddressStream>(ResourceTable::NullParent, Query);
  std::thread AsyncTask(
      WasiSocketsResolveAddressesBlock, std::ref(Env),
      std::ref(*Env.Table.GetById<ResolveAddressStream>(ResHandle)));
  AsyncTask.detach();
  RetBuf->Val.Res = ResHandle;
  return SocketsOk(*RetBuf);
}

Expect<void> ResolveNextAddress::body(const Runtime::CallingFrame &Frame,
                                      uint32_t Handle, uint32_t RetBufPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto RetBuf =
      MemInst->getPointer<__wasi_sockets_ret_t<__wasi_sockets_ip_address_t> *>(
          RetBufPtr);
  if (RetBuf == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  auto *Data = Env.Table.GetById<ResolveAddressStream>(Handle);
  if (Data == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  if (!Data->IsReady) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_WOULD_BLOCK);
  }

  if (Data->Resolved.size() == 0) {
    return SocketsErr(*RetBuf, __WASI_SOCKETS_ERRNO_NAME_UNRESOLVABLE);
  }

  RetBuf->Val.Res = Data->Resolved.back();
  Data->Resolved.pop_back();
  return SocketsOk(*RetBuf);
}

Expect<void> ResolveDrop::body(const Runtime::CallingFrame &, uint32_t Handle) {
  auto *Data = Env.Table.GetById<ResolveAddressStream>(Handle);
  if (Data == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
  }

  if (Data->IsReady) {
    Env.Table.Drop(Handle);
  } // TODO: can not remove because it is still used in the other thread

  return {};
}
} // namespace IpNameLookup
} // namespace WasiSocket
} // namespace Host
} // namespace WasmEdge
