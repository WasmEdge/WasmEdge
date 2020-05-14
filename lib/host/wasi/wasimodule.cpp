// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasimodule.h"
#include "host/wasi/wasifunc.h"

#include <memory>

namespace SSVM {
namespace Host {

WasiModule::WasiModule() : ImportObject("wasi_snapshot_preview1") {
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
  addHostFunc("fd_prestat_dir_name",
              std::make_unique<WasiFdPrestatDirName>(Env));
  addHostFunc("fd_prestat_get", std::make_unique<WasiFdPrestatGet>(Env));
  addHostFunc("fd_read", std::make_unique<WasiFdRead>(Env));
  addHostFunc("fd_seek", std::make_unique<WasiFdSeek>(Env));
  addHostFunc("fd_write", std::make_unique<WasiFdWrite>(Env));
  addHostFunc("path_open", std::make_unique<WasiPathOpen>(Env));
  addHostFunc("proc_exit", std::make_unique<WasiProcExit>(Env));
  addHostFunc("random_get", std::make_unique<WasiRandomGet>(Env));
}

} // namespace Host
} // namespace SSVM
