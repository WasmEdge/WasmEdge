// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/node.h - Shared node operations ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the node operations of the 0.2 and 0.3 hosts.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/span.h"
#include "host/wasi/error.h"
#include "host/wasi/vinode.h"
#include "runtime/component/callingframe.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// The file status of the open node Node.
inline WASI::WasiExpect<__wasi_filestat_t>
getFdFilestat(WASI::VINode &Node) noexcept {
  __wasi_filestat_t Stat;
  std::memset(&Stat, 0, sizeof(Stat));
  EXPECTED_TRY(Node.fdFilestatGet(Stat));
  return Stat;
}

/// The file status of Path under the directory node Node.
inline WASI::WasiExpect<__wasi_filestat_t>
getPathFilestat(std::shared_ptr<WASI::VINode> Node,
                __wasi_lookupflags_t LookupFlags,
                std::string_view Path) noexcept {
  __wasi_filestat_t Stat;
  std::memset(&Stat, 0, sizeof(Stat));
  EXPECTED_TRY(
      WASI::VINode::pathFilestatGet(std::move(Node), Path, LookupFlags, Stat));
  return Stat;
}

/// Write all of Data to a connected socket node.
inline WASI::WasiExpect<void> sendAll(WASI::VINode &Node,
                                      Span<const uint8_t> Data) noexcept {
  while (!Data.empty()) {
    std::array<Span<const uint8_t>, 1> IOVs{Data};
    __wasi_size_t Sent = 0;
    EXPECTED_TRY(Node.sockSend(IOVs, static_cast<__wasi_siflags_t>(0), Sent));
    if (Sent == 0) {
      return WASI::WasiUnexpect(__WASI_ERRNO_PIPE);
    }
    Data = Data.subspan(Sent);
  }
  return {};
}

/// Park Frame until Node is ready for reading or writing; a handle the OS
/// cannot wait on, a non-socket on Windows, is used at once.
inline Expect<void> waitReady(Runtime::Component::CallingFrame &Frame,
                              const WASI::VINode &Node, bool Write) noexcept {
#if WASMEDGE_OS_WINDOWS
  __wasi_fdstat_t Stat;
  if (!Node.fdFdstatGet(Stat) ||
      (Stat.fs_filetype != __WASI_FILETYPE_SOCKET_STREAM &&
       Stat.fs_filetype != __WASI_FILETYPE_SOCKET_DGRAM)) {
    return {};
  }
#endif
  auto Handle = Node.getNativeHandler();
  if (!Handle) {
    return {};
  }
  return Write ? Frame.waitWritable(*Handle) : Frame.waitReadable(*Handle);
}

/// Whether a non-blocking operation found nothing to do yet.
inline bool isPending(__wasi_errno_t Errno) noexcept {
  return Errno == __WASI_ERRNO_AGAIN || Errno == __WASI_ERRNO_INPROGRESS;
}

/// Connect the non-blocking socket Node, parking until it completes.
inline Expect<WASI::WasiExpect<void>>
sockConnect(Runtime::Component::CallingFrame &Frame, WASI::VINode &Node,
            __wasi_address_family_t Family, Span<const uint8_t> Address,
            uint16_t Port) noexcept {
  auto Res = Node.sockConnect(Family, Address, Port);
  if (Res || !isPending(Res.error())) {
    return Expect<WASI::WasiExpect<void>>(std::in_place, Res);
  }
  EXPECTED_TRY(waitReady(Frame, Node, true));
  if (Frame.isCancelled()) {
    return Expect<WASI::WasiExpect<void>>(
        std::in_place,
        WASI::WasiExpect<void>(WASI::WasiUnexpect(__WASI_ERRNO_CANCELED)));
  }
  int32_t Error = 0;
  Span<uint8_t> Flag(reinterpret_cast<uint8_t *>(&Error), sizeof(Error));
  if (auto Got = Node.sockGetOpt(__WASI_SOCK_OPT_LEVEL_SOL_SOCKET,
                                 __WASI_SOCK_OPT_SO_ERROR, Flag);
      !Got) {
    return Expect<WASI::WasiExpect<void>>(
        std::in_place, WASI::WasiExpect<void>(WASI::WasiUnexpect(Got.error())));
  }
  if (Error != 0) {
    return Expect<WASI::WasiExpect<void>>(
        std::in_place, WASI::WasiExpect<void>(WASI::WasiUnexpect(
                           static_cast<__wasi_errno_t>(Error))));
  }
  return Expect<WASI::WasiExpect<void>>(std::in_place,
                                        WASI::WasiExpect<void>{});
}

/// Receive into Buffer from the non-blocking socket Node, parking until data
/// or the end arrives.
inline Expect<WASI::WasiExpect<__wasi_size_t>>
sockRecv(Runtime::Component::CallingFrame &Frame, WASI::VINode &Node,
         Span<uint8_t> Buffer) noexcept {
  while (true) {
    std::array<Span<uint8_t>, 1> IOVs{Buffer};
    __wasi_size_t Count = 0;
    __wasi_roflags_t RoFlags = static_cast<__wasi_roflags_t>(0);
    auto Res =
        Node.sockRecv(IOVs, static_cast<__wasi_riflags_t>(0), Count, RoFlags);
    if (Res) {
      return Expect<WASI::WasiExpect<__wasi_size_t>>(
          std::in_place, WASI::WasiExpect<__wasi_size_t>(Count));
    }
    if (!isPending(Res.error())) {
      return Expect<WASI::WasiExpect<__wasi_size_t>>(
          std::in_place,
          WASI::WasiExpect<__wasi_size_t>(WASI::WasiUnexpect(Res.error())));
    }
    EXPECTED_TRY(waitReady(Frame, Node, false));
    if (Frame.isCancelled()) {
      return Expect<WASI::WasiExpect<__wasi_size_t>>(
          std::in_place, WASI::WasiExpect<__wasi_size_t>(
                             WASI::WasiUnexpect(__WASI_ERRNO_CANCELED)));
    }
  }
}

/// Send all of Data through the non-blocking socket Node.
inline Expect<WASI::WasiExpect<void>>
sockSend(Runtime::Component::CallingFrame &Frame, WASI::VINode &Node,
         Span<const uint8_t> Data) noexcept {
  while (!Data.empty()) {
    std::array<Span<const uint8_t>, 1> IOVs{Data};
    __wasi_size_t Sent = 0;
    auto Res = Node.sockSend(IOVs, static_cast<__wasi_siflags_t>(0), Sent);
    if (!Res) {
      if (!isPending(Res.error())) {
        return Expect<WASI::WasiExpect<void>>(
            std::in_place,
            WASI::WasiExpect<void>(WASI::WasiUnexpect(Res.error())));
      }
      EXPECTED_TRY(waitReady(Frame, Node, true));
      if (Frame.isCancelled()) {
        return Expect<WASI::WasiExpect<void>>(
            std::in_place,
            WASI::WasiExpect<void>(WASI::WasiUnexpect(__WASI_ERRNO_CANCELED)));
      }
      continue;
    }
    if (Sent == 0) {
      return Expect<WASI::WasiExpect<void>>(
          std::in_place,
          WASI::WasiExpect<void>(WASI::WasiUnexpect(__WASI_ERRNO_PIPE)));
    }
    Data = Data.subspan(Sent);
  }
  return Expect<WASI::WasiExpect<void>>(std::in_place,
                                        WASI::WasiExpect<void>{});
}

/// Accept a connection on the non-blocking listening socket Node; the
/// connection is non-blocking too.
inline Expect<WASI::WasiExpect<std::shared_ptr<WASI::VINode>>>
sockAccept(Runtime::Component::CallingFrame &Frame,
           WASI::VINode &Node) noexcept {
  while (true) {
    auto Res = Node.sockAccept(__WASI_FDFLAGS_NONBLOCK);
    if (Res || !isPending(Res.error())) {
      return Expect<WASI::WasiExpect<std::shared_ptr<WASI::VINode>>>(
          std::in_place, Res);
    }
    EXPECTED_TRY(waitReady(Frame, Node, false));
    if (Frame.isCancelled()) {
      return Expect<WASI::WasiExpect<std::shared_ptr<WASI::VINode>>>(
          std::in_place, WASI::WasiExpect<std::shared_ptr<WASI::VINode>>(
                             WASI::WasiUnexpect(__WASI_ERRNO_CANCELED)));
    }
  }
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
