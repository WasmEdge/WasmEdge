// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi/core.h"

#include <algorithm>
#include <mutex>
#include <boost/align/aligned_allocator.hpp>
#include <dirent.h>
#include <fcntl.h>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace SSVM {
namespace Host {

class WasiEnvironment {
public:
  struct DirFdStat {
    DIR *Dir;
    uint64_t Cookie = 0;
    std::vector<uint8_t, boost::alignment::aligned_allocator<
                             uint8_t, alignof(__wasi_dirent_t)>>
        Buffer;
    DirFdStat(DIR *D) noexcept : Dir(D) {}
    ~DirFdStat() noexcept { closedir(Dir); }
  };
  struct File {
    __wasi_fd_t Fd;
    __wasi_rights_t Rights;
    __wasi_rights_t InheritingRights;
    std::string Path;
    std::optional<DirFdStat> Dir;

    File(__wasi_fd_t F, __wasi_rights_t R, __wasi_rights_t IR,
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

  int32_t getStatus() const { return Status; }
  void setStatus(int32_t S) { Status = S; }
  std::vector<std::string> &getCmdArgs() { return CmdArgs; }
  std::vector<std::string_view> &getEnvirons() { return Environs; }
  int getExitCode() const { return ExitCode; }
  void setExitCode(int ExitCode) { this->ExitCode = ExitCode; }

  template <typename... Args> void emplaceFile(Args &&... args) {
    FileArray.emplace_back(std::forward<Args>(args)...);
  }
  std::vector<File>::iterator getFile(uint32_t Fd) noexcept {
    return std::find_if(FileArray.begin(), FileArray.end(),
                        [Fd](const File &File) { return File.Fd == Fd; });
  }
  std::vector<File>::iterator getFileEnd() noexcept { return FileArray.end(); }
  void eraseFile(std::vector<File>::iterator File) noexcept {
    FileArray.erase(File);
  }

private:
  int32_t Status;
  std::vector<std::string> CmdArgs;
  std::vector<std::string_view> Environs;
  std::vector<File> FileArray;
  int ExitCode = 0;
};

} // namespace Host
} // namespace SSVM
