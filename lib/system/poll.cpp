// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "system/poll.h"

#include "common/defines.h"

#include <algorithm>
#include <thread>
#include <vector>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <cerrno>
#include <poll.h>
#elif WASMEDGE_OS_WINDOWS
#include <winsock2.h>
#endif

namespace WasmEdge {

void Poll::wait(
    Span<PollEntry> Entries,
    std::optional<std::chrono::steady_clock::time_point> Deadline) noexcept {
  for (auto &Watched : Entries) {
    Watched.Ready = false;
  }
  int Timeout = -1;
  if (Deadline.has_value()) {
    const auto Left = std::chrono::ceil<std::chrono::milliseconds>(
        *Deadline - std::chrono::steady_clock::now());
    Timeout = static_cast<int>(std::max<int64_t>(Left.count(), 0));
  }
  if (Entries.empty()) {
    if (Deadline.has_value()) {
      std::this_thread::sleep_until(*Deadline);
    }
    return;
  }
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  std::vector<struct pollfd> Fds(Entries.size());
  for (size_t I = 0; I < Entries.size(); ++I) {
    Fds[I].fd = static_cast<int>(Entries[I].Handle);
    Fds[I].events = Entries[I].Write ? POLLOUT : POLLIN;
    Fds[I].revents = 0;
  }
  int Res = 0;
  do {
    Res = ::poll(Fds.data(), static_cast<nfds_t>(Fds.size()), Timeout);
  } while (Res < 0 && errno == EINTR);
#elif WASMEDGE_OS_WINDOWS
  std::vector<WSAPOLLFD> Fds(Entries.size());
  for (size_t I = 0; I < Entries.size(); ++I) {
    Fds[I].fd = static_cast<SOCKET>(Entries[I].Handle);
    Fds[I].events = Entries[I].Write ? POLLWRNORM : POLLRDNORM;
    Fds[I].revents = 0;
  }
  const int Res =
      ::WSAPoll(Fds.data(), static_cast<ULONG>(Fds.size()), Timeout);
#endif
  if (Res <= 0) {
    return;
  }
  for (size_t I = 0; I < Entries.size(); ++I) {
    Entries[I].Ready = Fds[I].revents != 0;
  }
}

} // namespace WasmEdge
