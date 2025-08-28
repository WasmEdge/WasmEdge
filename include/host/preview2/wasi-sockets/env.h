#pragma once

#include "host/preview2/wasi-sockets/api.h"
#include "host/preview2/wasi-sockets/resource_table.h"
#include "host/wasi/environ.h"
#include <functional>

namespace WasmEdge {
namespace Host {

namespace WasiSocket {
namespace Network {
struct Network : public ResourceElementBase {
  bool AllowIPNameCheck;
  bool
  CheckAddress([[maybe_unused]] const __wasi_sockets_ip_address_t &Address) {
    return true;
  }
  std::function<bool(const __wasi_sockets_ip_address_t &)> GetChecker() {
    return [this](const __wasi_sockets_ip_address_t &Addr) -> bool {
      return this->CheckAddress(Addr);
    };
  }
  Network() : ResourceElementBase() {}
};
} // namespace Network
namespace UDP {
enum class SocketState { Default, BindStarted, Bound, Connected };

struct UDPSocket : public ResourceElementBase {
  __wasi_fd_t Fd;
  __wasi_sockets_ip_address_family_t AddressFamily;
  SocketState State;
  bool IPv6Only;
  std::function<bool(const __wasi_sockets_ip_address_t &)> AddressChecker;

  std::list<HandleT> Children;
  std::list<HandleT> *GetChildren() override { return &Children; };

  static bool DefaultAddressChecker(const __wasi_sockets_ip_address_t &) {
    return true;
  };

  UDPSocket(__wasi_fd_t Fd, __wasi_sockets_ip_address_family_t AddressFamily,
            SocketState State)
      : ResourceElementBase(), Fd(Fd), AddressFamily(AddressFamily),
        State(State), IPv6Only(true), AddressChecker(DefaultAddressChecker) {}
};

struct IncomingDatagramStream : public ResourceElementBase {
  __wasi_fd_t Fd;
  __wasi_sockets_ip_socket_address_t RemoteAddress;

  IncomingDatagramStream(__wasi_fd_t Fd,
                         __wasi_sockets_ip_socket_address_t RemoteAddress)
      : ResourceElementBase(), Fd(Fd), RemoteAddress(RemoteAddress) {}
};

struct OutgoingDatagramStream : public ResourceElementBase {
  __wasi_fd_t Fd;
  __wasi_sockets_ip_socket_address_t RemoteAddress;
  __wasi_sockets_ip_address_family_t AddressFamily;

  OutgoingDatagramStream(__wasi_fd_t Fd,
                         __wasi_sockets_ip_socket_address_t RemoteAddress,
                         __wasi_sockets_ip_address_family_t AddressFamily)
      : ResourceElementBase(), Fd(Fd), RemoteAddress(RemoteAddress),
        AddressFamily(AddressFamily) {}
};

} // namespace UDP
namespace TCP {
enum class TCPSocketState {
  Default,
  BindStarted,
  Bound,
  ListenStarted,
  Listening,
  Connecting,
  ConnectReady,
  ConnectFailed,
  Connected
};

struct TCPSocket : public ResourceElementBase {
  __wasi_fd_t Fd;
  __wasi_sockets_ip_address_family_t AddressFamily;
  std::atomic<TCPSocketState> State;
  int32_t ListenBacklogSize;
  bool IPv6Only;

  // According
  // https://github.com/rust-lang/rust/blob/master/library/std/src/sys_common/net.rs
  // ListenBacklogSize default for most platform is 128
  TCPSocket(__wasi_fd_t Fd, __wasi_sockets_ip_address_family_t AddressFamily,
            TCPSocketState State)
      : Fd(Fd), AddressFamily(AddressFamily), State(State),
        ListenBacklogSize(128), IPv6Only(false) {}
};
} // namespace TCP
namespace IpNameLookup {
struct ResolveAddressStream : public ResourceElementBase {
  std::atomic<bool> IsReady;
  std::string Query;
  std::vector<__wasi_sockets_ip_address_t> Resolved;
  bool Invalid;

  ResolveAddressStream(std::string_view Query)
      : IsReady(false), Query(Query), Resolved(), Invalid(false) {}
};
} // namespace IpNameLookup
} // namespace WasiSocket

class WasiSocketsEnvironment : public WASI::Environ {
public:
  WasiSocketsEnvironment() noexcept {}
  ResourceTable Table;
};

} // namespace Host
} // namespace WasmEdge
