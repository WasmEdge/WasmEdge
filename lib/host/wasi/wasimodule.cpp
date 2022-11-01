// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  addHostFunc("poll_oneoff", std::make_unique<WasiPollOneoff>(Env));
  addHostFunc("epoll_oneoff", std::make_unique<WasiEpollOneoff>(Env));
  addHostFunc("proc_exit", std::make_unique<WasiProcExit>(Env));
  addHostFunc("proc_raise", std::make_unique<WasiProcRaise>(Env));
  addHostFunc("sched_yield", std::make_unique<WasiSchedYield>(Env));
  addHostFunc("random_get", std::make_unique<WasiRandomGet>(Env));
  addHostFunc("sock_open", std::make_unique<WasiSockOpen>(Env));
  addHostFunc("sock_bind", std::make_unique<WasiSockBind>(Env));
  addHostFunc("sock_connect", std::make_unique<WasiSockConnect>(Env));
  addHostFunc("sock_listen", std::make_unique<WasiSockListen>(Env));
  addHostFunc("sock_accept", std::make_unique<WasiSockAccept>(Env));
  addHostFunc("sock_recv", std::make_unique<WasiSockRecv>(Env));
  addHostFunc("sock_recv_from", std::make_unique<WasiSockRecvFrom>(Env));
  addHostFunc("sock_send", std::make_unique<WasiSockSend>(Env));
  addHostFunc("sock_send_to", std::make_unique<WasiSockSendTo>(Env));
  addHostFunc("sock_shutdown", std::make_unique<WasiSockShutdown>(Env));
  addHostFunc("sock_getsockopt", std::make_unique<WasiSockGetOpt>(Env));
  addHostFunc("sock_setsockopt", std::make_unique<WasiSockSetOpt>(Env));
  addHostFunc("sock_getlocaladdr", std::make_unique<WasiSockGetLocalAddr>(Env));
  addHostFunc("sock_getpeeraddr", std::make_unique<WasiSockGetPeerAddr>(Env));
  addHostFunc("sock_getaddrinfo", std::make_unique<WasiGetAddrinfo>(Env));
}

} // namespace Host
} // namespace WasmEdge
