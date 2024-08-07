// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "host/wasi/wasimodule.h"
#include "host/wasi/wasifunc.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiModule::WasiModule() : ModuleInstance("wasi_snapshot_preview1") {
  addHostFunc("args_get", std::make_unique<WasiArgsGet>(Env));
  addHostFunc("args_sizes_get", std::make_unique<WasiArgsSizesGet>(Env));
  addHostFunc("environ_get", std::make_unique<WasiEnvironGet>(Env));
  addHostFunc("environ_sizes_get", std::make_unique<WasiEnvironSizesGet>(Env));
  addHostFunc("clock_res_get", std::make_unique<WasiClockResGet>(Env));
  addHostFunc("clock_time_get", std::make_unique<WasiClockTimeGet>(Env));
  addHostFunc("fd_advise", std::make_unique<WasiFdAdvise>(Env));
  addHostFunc("fd_allocate", std::make_unique<WasiFdAllocate>(Env));
  addHostFunc("fd_close", std::make_unique<WasiFdClose>(Env));
  addHostFunc("fd_datasync", std::make_unique<WasiFdDatasync>(Env));
  addHostFunc("fd_fdstat_get", std::make_unique<WasiFdFdstatGet>(Env));
  addHostFunc("fd_fdstat_set_flags",
              std::make_unique<WasiFdFdstatSetFlags>(Env));
  addHostFunc("fd_fdstat_set_rights",
              std::make_unique<WasiFdFdstatSetRights>(Env));
  addHostFunc("fd_filestat_get", std::make_unique<WasiFdFilestatGet>(Env));
  addHostFunc("fd_filestat_set_size",
              std::make_unique<WasiFdFilestatSetSize>(Env));
  addHostFunc("fd_filestat_set_times",
              std::make_unique<WasiFdFilestatSetTimes>(Env));
  addHostFunc("fd_pread", std::make_unique<WasiFdPread>(Env));
  addHostFunc("fd_prestat_get", std::make_unique<WasiFdPrestatGet>(Env));
  addHostFunc("fd_prestat_dir_name",
              std::make_unique<WasiFdPrestatDirName>(Env));
  addHostFunc("fd_pwrite", std::make_unique<WasiFdPwrite>(Env));
  addHostFunc("fd_read", std::make_unique<WasiFdRead>(Env));
  addHostFunc("fd_readdir", std::make_unique<WasiFdReadDir>(Env));
  addHostFunc("fd_renumber", std::make_unique<WasiFdRenumber>(Env));
  addHostFunc("fd_seek", std::make_unique<WasiFdSeek>(Env));
  addHostFunc("fd_sync", std::make_unique<WasiFdSync>(Env));
  addHostFunc("fd_tell", std::make_unique<WasiFdTell>(Env));
  addHostFunc("fd_write", std::make_unique<WasiFdWrite>(Env));
  addHostFunc("path_create_directory",
              std::make_unique<WasiPathCreateDirectory>(Env));
  addHostFunc("path_filestat_get", std::make_unique<WasiPathFilestatGet>(Env));
  addHostFunc("path_filestat_set_times",
              std::make_unique<WasiPathFilestatSetTimes>(Env));
  addHostFunc("path_link", std::make_unique<WasiPathLink>(Env));
  addHostFunc("path_open", std::make_unique<WasiPathOpen>(Env));
  addHostFunc("path_readlink", std::make_unique<WasiPathReadLink>(Env));
  addHostFunc("path_remove_directory",
              std::make_unique<WasiPathRemoveDirectory>(Env));
  addHostFunc("path_rename", std::make_unique<WasiPathRename>(Env));
  addHostFunc("path_symlink", std::make_unique<WasiPathSymlink>(Env));
  addHostFunc("path_unlink_file", std::make_unique<WasiPathUnlinkFile>(Env));
  addHostFunc("poll_oneoff",
              std::make_unique<WasiPollOneoff<WASI::TriggerType::Level>>(Env));
  addHostFunc("epoll_oneoff",
              std::make_unique<WasiPollOneoff<WASI::TriggerType::Edge>>(Env));
  addHostFunc("proc_exit", std::make_unique<WasiProcExit>(Env));
  addHostFunc("proc_raise", std::make_unique<WasiProcRaise>(Env));
  addHostFunc("sched_yield", std::make_unique<WasiSchedYield>(Env));
  addHostFunc("random_get", std::make_unique<WasiRandomGet>(Env));
  // To make the socket API compatible with the old one,
  // we will duplicate all the API to V1 and V2.
  // The V1 presents the original behavior before 0.12 release.
  // On the other hand, the V2 presents the new behavior including
  // the sock_accept is following the WASI spec, some of the API
  // use a larger size for handling complex address type, e.g.
  // AF_UNIX.
  // By default, we will register V1 first, if the signatures are
  // not the same as the wasm application imported, then V2 will
  // replace instead.
  addHostFunc("sock_open", std::make_unique<WasiSockOpenV1>(Env));
  addHostFunc("sock_bind", std::make_unique<WasiSockBindV1>(Env));
  addHostFunc("sock_connect", std::make_unique<WasiSockConnectV1>(Env));
  addHostFunc("sock_listen", std::make_unique<WasiSockListenV1>(Env));
  addHostFunc("sock_accept", std::make_unique<WasiSockAcceptV1>(Env));
  addHostFunc("sock_recv", std::make_unique<WasiSockRecvV1>(Env));
  addHostFunc("sock_recv_from", std::make_unique<WasiSockRecvFromV1>(Env));
  addHostFunc("sock_send", std::make_unique<WasiSockSendV1>(Env));
  addHostFunc("sock_send_to", std::make_unique<WasiSockSendToV1>(Env));
  addHostFunc("sock_accept_v2", std::make_unique<WasiSockAcceptV2>(Env));
  addHostFunc("sock_open_v2", std::make_unique<WasiSockOpenV2>(Env));
  addHostFunc("sock_bind_v2", std::make_unique<WasiSockBindV2>(Env));
  addHostFunc("sock_connect_v2", std::make_unique<WasiSockConnectV2>(Env));
  addHostFunc("sock_listen_v2", std::make_unique<WasiSockListenV2>(Env));
  addHostFunc("sock_recv_v2", std::make_unique<WasiSockRecvV2>(Env));
  addHostFunc("sock_recv_from_v2", std::make_unique<WasiSockRecvFromV2>(Env));
  addHostFunc("sock_send_v2", std::make_unique<WasiSockSendV2>(Env));
  addHostFunc("sock_send_to_v2", std::make_unique<WasiSockSendToV2>(Env));
  addHostFunc("sock_shutdown", std::make_unique<WasiSockShutdown>(Env));
  addHostFunc("sock_getsockopt", std::make_unique<WasiSockGetOpt>(Env));
  addHostFunc("sock_setsockopt", std::make_unique<WasiSockSetOpt>(Env));
  addHostFunc("sock_getlocaladdr",
              std::make_unique<WasiSockGetLocalAddrV1>(Env));
  addHostFunc("sock_getpeeraddr", std::make_unique<WasiSockGetPeerAddrV1>(Env));
  addHostFunc("sock_getlocaladdr_v2",
              std::make_unique<WasiSockGetLocalAddrV2>(Env));
  addHostFunc("sock_getpeeraddr_v2",
              std::make_unique<WasiSockGetPeerAddrV2>(Env));
  addHostFunc("sock_getaddrinfo", std::make_unique<WasiSockGetAddrinfo>(Env));
}

} // namespace Host
} // namespace WasmEdge
