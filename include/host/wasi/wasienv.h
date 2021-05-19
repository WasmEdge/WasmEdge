// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "wasi/api.hpp"

#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/align/aligned_allocator.hpp>

namespace WasmEdge {
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
    int HostFd;
    bool IsPreopened;
    __wasi_rights_t Rights;
    __wasi_rights_t InheritingRights;
    std::string Path;
    std::optional<DirFdStat> Dir;

    File(int F, bool I, __wasi_rights_t R, __wasi_rights_t IR,
         std::string_view P)
        : HostFd(F), IsPreopened(I), Rights(R), InheritingRights(IR), Path(P) {}
    bool checkRights(__wasi_rights_t RequiredRights,
                     __wasi_rights_t RequiredInheritingRights =
                         static_cast<__wasi_rights_t>(0)) const {
      return (Rights & RequiredRights) == RequiredRights &&
             (InheritingRights & RequiredInheritingRights) ==
                 RequiredInheritingRights;
    }
  };

  WasiEnvironment();
  virtual ~WasiEnvironment() noexcept;

  void init(Span<const std::string> Dirs, std::string ProgramName,
            Span<const std::string> Args, Span<const std::string> Envs);
  void fini() noexcept;

  const std::vector<std::string> &getCmdArgs() const { return CmdArgs; }
  const std::vector<std::string> &getEnvirons() const { return Environs; }
  int getExitCode() const { return ExitCode; }
  void setExitCode(int ExitCode) { this->ExitCode = ExitCode; }

  template <typename... Args> void emplaceFile(__wasi_fd_t Fd, Args &&...args) {
    FileMap.emplace(std::piecewise_construct, std::forward_as_tuple(Fd),
                    std::forward_as_tuple(std::forward<Args>(args)...));
  }
  using FileIterator = std::map<__wasi_fd_t, File>::iterator;
  FileIterator getFile(__wasi_fd_t Fd) noexcept { return FileMap.find(Fd); }
  FileIterator getFileEnd() noexcept { return FileMap.end(); }
  void eraseFile(FileIterator File) noexcept { FileMap.erase(File); }
  __wasi_fd_t getNewFd() noexcept { return FileMap.rbegin()->first + 1; }
  void changeFd(FileIterator File, uint32_t Fd) {
    auto Node = FileMap.extract(File);
    Node.key() = Fd;
    FileMap.insert(std::move(Node));
  }

private:
  std::vector<std::string> CmdArgs;
  std::vector<std::string> Environs;
  std::map<__wasi_fd_t, File> FileMap;
  int ExitCode = 0;
};

} // namespace Host
} // namespace WasmEdge
