// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi/core.h"

#include <fcntl.h>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace SSVM {
namespace Host {

class WasiEnvironment {
public:
  struct PreStat {
    __wasi_fd_t Fd;
    __wasi_rights_t Rights;
    __wasi_rights_t InheritingRights;
    std::string Path;
    PreStat(__wasi_fd_t F, __wasi_rights_t R, __wasi_rights_t IR,
            std::string_view P)
        : Fd(F), Rights(R), InheritingRights(IR), Path(P) {}
    bool checkRights(__wasi_rights_t RequiredRights,
                     __wasi_rights_t RequiredInheritingRights = 0) const {
      return (Rights & RequiredRights) == RequiredRights &&
             (InheritingRights & RequiredInheritingRights) ==
                 RequiredInheritingRights;
    }
  };

  WasiEnvironment();
  virtual ~WasiEnvironment() noexcept;

  void clear() { CmdArgs.clear(); }

  int32_t getStatus() const { return Status; }
  void setStatus(int32_t S) { Status = S; }
  std::vector<std::string> &getCmdArgs() { return CmdArgs; }
  std::vector<PreStat> &getPreStats() { return PreStats; }
  int getExitCode() const { return ExitCode; }
  void setExitCode(int ExitCode) { this->ExitCode = ExitCode; }

private:
  int32_t Status;
  std::vector<std::string> CmdArgs;
  std::vector<PreStat> PreStats;
  int ExitCode = 0;
};

} // namespace Host
} // namespace SSVM
