// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi/core.h"

#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>

namespace SSVM {
namespace Host {

class WasiEnvironment {
public:
  struct PreStat {
    int32_t Fd;
    uint8_t Type;
    std::vector<unsigned char> Path;
    PreStat(int32_t F, uint8_t T, std::vector<unsigned char> P)
        : Fd(F), Type(T), Path(std::move(P)) {}
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